// The Windows half of platform/process.h: the argument list quoted into one
// command line, CreateProcessW inside a job object, a zero-timeout wait to poll,
// and TerminateJobObject to end it.
//
// THE COMMAND LINE IS QUOTED BY THE CommandLineToArgvW RULES, which is what the
// C runtime of every program we start parses it back with: an argument that is
// empty or holds a space, tab or quote is wrapped in quotes; inside, a quote is
// escaped with a backslash, and backslashes are doubled only where they stand
// before a quote, the closing one included. It is built as UTF-8 in scratch,
// then converted to UTF-16 in scratch through platform/src/wide_win32.h,
// sized so it always fits: a UTF-16 text never has more units than its
// UTF-8 has bytes. The output path crosses the same way, in a MAX_PATH stack
// buffer; one that does not fit fails as an open does (ADR-0248).
//
// THE CHILD STARTS SUSPENDED AND JOINS THE JOB BEFORE IT RUNS, so nothing it
// starts can escape the job. The job is kill-on-close: if the editor dies
// without calling _end, closing its last handle ends the group anyway.
//
// The child inherits the caller's standard handles through STARTF_USESTDHANDLES
// with handle inheritance on, so its output lands wherever the editor's does.
// An output path is opened first as an inheritable handle for append, created
// when missing, and set as both hStdOutput and hStdError; the parent closes its
// copy after CreateProcessW. Written and built on Linux only (ADR-0130).
#include <platform/process.h>

#include "wide_win32.h"

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>

#include <string.h>

// Writes one argument quoted into to, returns the bytes written. to has room
// for 2 * strlen(argument) + 3.
static size_t quote_argument(const char *argument, char *to)
{
	size_t written = 0;
	size_t backslashes = 0;

	if (argument[0] != '\0' && strpbrk(argument, " \t\"") == NULL) {
		memcpy(to, argument, strlen(argument));
		return strlen(argument);
	}
	to[written++] = '"';
	for (const char *c = argument; *c != '\0'; c++) {
		if (*c == '\\') {
			backslashes++;
		} else if (*c == '"') {
			// The run already written once is doubled, plus one for the quote.
			for (size_t i = 0; i < backslashes + 1; i++)
				to[written++] = '\\';
			backslashes = 0;
		} else {
			backslashes = 0;
		}
		to[written++] = *c;
	}
	// The trailing run, already written once, is doubled before the closing quote.
	for (size_t i = 0; i < backslashes; i++)
		to[written++] = '\\';
	to[written++] = '"';
	return written;
}

static wchar_t *command_line_of(const char *const *argv, voe_base_arena *scratch)
{
	size_t capacity = 1;
	size_t length = 0;
	char *line;
	wchar_t *wide;

	for (size_t i = 0; argv[i] != NULL; i++)
		capacity += 2 * strlen(argv[i]) + 4;
	line = voe_base_arena_push(scratch, capacity);
	for (size_t i = 0; argv[i] != NULL; i++) {
		if (i > 0)
			line[length++] = ' ';
		length += quote_argument(argv[i], line + length);
	}
	line[length] = '\0';

	wide = voe_base_arena_push(scratch, (length + 1) * sizeof(*wide));
	VOE_BASE_ASSERT(voe_platform_wide_from_utf8(line, wide, (int)(length + 1)),
			"a command line did not fit its own UTF-8 length in UTF-16");
	return wide;
}

// An inheritable append handle on output, or INVALID_HANDLE_VALUE, reported.
static HANDLE output_handle_of(const char *output)
{
	SECURITY_ATTRIBUTES inherit = { sizeof(inherit), NULL, TRUE };
	wchar_t wide[MAX_PATH];
	HANDLE file = INVALID_HANDLE_VALUE;
	DWORD error = ERROR_FILENAME_EXCED_RANGE;

	VOE_BASE_DEBUG_ASSERT(output != NULL, "redirecting to no output");
	if (voe_platform_wide_from_utf8(output, wide, MAX_PATH)) {
		file = CreateFileW(wide, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
				   &inherit, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		error = GetLastError();
	}
	if (file == INVALID_HANDLE_VALUE)
		VOE_BASE_ERROR("platform", "cannot open %s for a program's output: error %lu",
			       output, (unsigned long)error);
	return file;
}

bool voe_platform_process_start(const char *const *argv, const char *output,
				voe_base_arena *scratch, voe_platform_process *out)
{
	HANDLE file = INVALID_HANDLE_VALUE;
	struct voe_base_arena_mark mark;
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = { 0 };
	STARTUPINFOW startup = { 0 };
	PROCESS_INFORMATION started = { 0 };
	HANDLE job;
	BOOL created;

	VOE_BASE_DEBUG_ASSERT(argv != NULL && argv[0] != NULL, "starting a program with no name");
	VOE_BASE_DEBUG_ASSERT(scratch != NULL, "starting a program with no scratch arena");
	VOE_BASE_DEBUG_ASSERT(out != NULL && out->handle == 0, "starting into a struct already running");

	if (output != NULL) {
		file = output_handle_of(output);
		if (file == INVALID_HANDLE_VALUE)
			return false;
	}
	job = CreateJobObjectW(NULL, NULL);
	VOE_BASE_ASSERT(job != NULL, "CreateJobObjectW failed");
	limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	VOE_BASE_ASSERT(SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits,
						sizeof(limits)),
			"SetInformationJobObject refused kill-on-close");

	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;
	startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	startup.hStdOutput = file != INVALID_HANDLE_VALUE ? file : GetStdHandle(STD_OUTPUT_HANDLE);
	startup.hStdError = file != INVALID_HANDLE_VALUE ? file : GetStdHandle(STD_ERROR_HANDLE);

	mark = voe_base_arena_mark(scratch);
	created = CreateProcessW(NULL, command_line_of(argv, scratch), NULL, NULL, TRUE,
				 CREATE_SUSPENDED, NULL, NULL, &startup, &started);
	voe_base_arena_rewind(scratch, mark);
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	if (!created) {
		VOE_BASE_ERROR("platform", "cannot start %s: error %lu", argv[0],
			       (unsigned long)GetLastError());
		CloseHandle(job);
		return false;
	}
	if (!AssignProcessToJobObject(job, started.hProcess)) {
		VOE_BASE_ERROR("platform", "cannot put %s in a job: error %lu", argv[0],
			       (unsigned long)GetLastError());
		TerminateProcess(started.hProcess, 1);
		CloseHandle(started.hThread);
		CloseHandle(started.hProcess);
		CloseHandle(job);
		return false;
	}
	ResumeThread(started.hThread);
	CloseHandle(started.hThread);

	out->handle = (uint64_t)(uintptr_t)started.hProcess;
	out->job = (uint64_t)(uintptr_t)job;
	return true;
}

static void release(voe_platform_process *process)
{
	CloseHandle((HANDLE)(uintptr_t)process->handle);
	CloseHandle((HANDLE)(uintptr_t)process->job);
	*process = (voe_platform_process){ 0 };
}

voe_platform_process_state voe_platform_process_poll(voe_platform_process *process,
						     int *exit_code)
{
	DWORD code;

	VOE_BASE_ASSERT(process != NULL && process->handle != 0, "polling a process that is not running");
	VOE_BASE_DEBUG_ASSERT(exit_code != NULL, "polling a process with nowhere to put its exit code");

	if (WaitForSingleObject((HANDLE)(uintptr_t)process->handle, 0) != WAIT_OBJECT_0)
		return VOE_PLATFORM_PROCESS_RUNNING;

	VOE_BASE_ASSERT(GetExitCodeProcess((HANDLE)(uintptr_t)process->handle, &code),
			"GetExitCodeProcess refused a process this one started");
	*exit_code = (int)code;
	release(process);
	return VOE_PLATFORM_PROCESS_ENDED;
}

void voe_platform_process_end(voe_platform_process *process)
{
	VOE_BASE_DEBUG_ASSERT(process != NULL, "ending a NULL process");
	if (process->handle == 0)
		return;

	TerminateJobObject((HANDLE)(uintptr_t)process->job, 1);
	WaitForSingleObject((HANDLE)(uintptr_t)process->handle, INFINITE);
	release(process);
}
