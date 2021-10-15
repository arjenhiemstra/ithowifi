#pragma once

#if defined (ENABLE_SERIAL)
#  define D_LOG(...) printf(__VA_ARGS__)
#else
#  define D_LOG(...)
#endif
