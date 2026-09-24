// The Linux half of platform/process.h: posix_spawnp into a new process group,
// waitpid with WNOHANG to poll, and SIGTERM to the group to end it.
//
// Linux only despite the name, as every _wayland file here is; see
// library_wayland.c for why the suffix says wayland.
//
// _GNU_SOURCE is what makes <unistd.h> declare environ, the child's inherited
// environment, so this file never spells out its char ** type (rule 6).
//
// THE GROUP IS SET BY THE SPAWN, NOT AFTERWARDS. POSIX_SPAWN_SETPGROUP with
// group 0 makes the child the leader of a group named by its own pid before it
// execs, so there is no window in which it has started something that is not in
// the group. Ending is kill(-pid, SIGTERM): the child, Ninja and the compiler
// all get it. Only the child is waited for; the rest are its to reap.
//
// A START THAT CANNOT EXEC IS A FALSE RETURN. glibc's posix_spawnp reports a
// failed exec (a name not on PATH) as its return value, not as a child that
// exits 127, so a missing program is reported here and never polled.
//
// A death by signal is reported as 128 plus the signal number, the shell's
// convention, which is never zero.
#define _GNU_SOURCE
#include <platform/process.h>

#include <base/assert.h>
#include <base/report.h>

#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int exit_code_of(int status)
{
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	if (WIFSIGNALED(status))
		return 128 + WTERMSIG(status);
	return 1;
}

bool voe_platform_process_start(const char *const *argv, voe_base_arena *scratch,
				voe_platform_process *out)
{
	posix_spawnattr_t attributes;
	char *const *arguments;
	pid_t pid;
	int failed;

	VOE_BASE_DEBUG_ASSERT(argv != NULL && argv[0] != NULL, "starting a program with no name");
	VOE_BASE_DEBUG_ASSERT(scratch != NULL, "starting a program with no scratch arena");
	(void)scratch; // only Windows builds a command line in it
	VOE_BASE_DEBUG_ASSERT(out != NULL && out->handle == 0, "starting into a struct already running");

	// posix_spawnp takes char *const[] for a list it promises not to change;
	// the copy drops a const that C cannot express on the parameter.
	memcpy(&arguments, &argv, sizeof(arguments));

	VOE_BASE_ASSERT(posix_spawnattr_init(&attributes) == 0, "out of memory starting a program");
	VOE_BASE_ASSERT(posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP) == 0 &&
				posix_spawnattr_setpgroup(&attributes, 0) == 0,
			"posix_spawnattr refused a process group");
	failed = posix_spawnp(&pid, argv[0], NULL, &attributes, arguments, environ);
	posix_spawnattr_destroy(&attributes);

	if (failed != 0) {
		VOE_BASE_ERROR("platform", "cannot start %s: %s", argv[0], strerror(failed));
		return false;
	}
	out->handle = (uint64_t)pid;
	out->job = 0;
	return true;
}

voe_platform_process_state voe_platform_process_poll(voe_platform_process *process,
						     int *exit_code)
{
	int status;
	pid_t ended;

	VOE_BASE_ASSERT(process != NULL && process->handle != 0, "polling a process that is not running");
	VOE_BASE_DEBUG_ASSERT(exit_code != NULL, "polling a process with nowhere to put its exit code");

	ended = waitpid((pid_t)process->handle, &status, WNOHANG);
	VOE_BASE_ASSERT(ended >= 0, "waitpid refused a child this process started");
	if (ended == 0)
		return VOE_PLATFORM_PROCESS_RUNNING;

	*exit_code = exit_code_of(status);
	*process = (voe_platform_process){ 0 };
	return VOE_PLATFORM_PROCESS_ENDED;
}

void voe_platform_process_end(voe_platform_process *process)
{
	int status;
	pid_t pid;

	VOE_BASE_DEBUG_ASSERT(process != NULL, "ending a NULL process");
	if (process->handle == 0)
		return;

	pid = (pid_t)process->handle;
	kill(-pid, SIGTERM);
	while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
		;
	*process = (voe_platform_process){ 0 };
}
