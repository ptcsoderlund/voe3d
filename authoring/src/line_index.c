// See the header.
#include "line_index.h"

#include <base/assert.h>

#include <string.h>

static bool blank(char c)
{
	return c == ' ' || c == '\t';
}

void voe_authoring_line_index(const char *text, size_t size,
			      const voe_assets_sectioned *doc,
			      uint32_t *out_section_line, uint32_t *out_key_line,
			      voe_authoring_span *out_key_span)
{
	uint32_t line = 1;
	uint32_t section = 0;
	uint32_t key = 0;
	size_t at = 0;

	VOE_BASE_ASSERT(text != NULL || size == 0, "indexing NULL text");
	VOE_BASE_ASSERT(doc != NULL, "indexing with no parsed document");

	while (at < size) {
		const char *newline = memchr(text + at, '\n', size - at);
		size_t end = newline != NULL ? (size_t)(newline - text) : size;
		size_t first = at;
		size_t last = end;

		if (last > first && text[last - 1] == '\r')
			last--;
		while (first < last && blank(text[first]))
			first++;
		while (last > first && blank(text[last - 1]))
			last--;

		if (first == last ||
		    (last - first >= 2 && text[first] == '/' &&
		     text[first + 1] == '/')) {
			// Blank or a comment.
		} else if (text[first] == '[') {
			VOE_BASE_ASSERT(section < doc->section_count,
					"more headers than the sectioned reader found");
			out_section_line[section++] = line;
		} else {
			VOE_BASE_ASSERT(key < doc->key_count,
					"more keys than the sectioned reader found");
			out_key_line[key] = line;
			if (out_key_span != NULL)
				out_key_span[key] = (voe_authoring_span){
					.bytes = text + first,
					.size = last - first,
				};
			key++;
		}

		at = end + 1;
		line++;
	}

	VOE_BASE_ASSERT(section == doc->section_count && key == doc->key_count,
			"the line walk and the sectioned reader disagree");
}
