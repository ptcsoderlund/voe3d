// The one function on base/error.h. It is a switch and not a table, because a
// table indexed by an enum is a silent wrong answer the day a code is inserted
// in the middle, and this switch is a build error on that same day.
#include <base/error.h>

const char *voe_base_error_string(voe_base_error error)
{
	switch (error) {
	case VOE_BASE_OK:
		return "no error";
	case VOE_BASE_ERROR_UNAVAILABLE:
		return "something this machine was supposed to provide is not installed";
	case VOE_BASE_ERROR_UNSUPPORTED:
		return "this machine cannot do what the engine requires";
	case VOE_BASE_ERROR_REFUSED:
		return "the system refused the operation";
	case VOE_BASE_ERROR_MALFORMED:
		return "the data is not what it claims to be";
	}

	return "unknown error";
}
