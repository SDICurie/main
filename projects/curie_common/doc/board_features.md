@page board_features  Board Features

If you run the same binary in slighly different boards, the bootloader will
expose those difference via the _features_ API. This API is accessible via two
headers, one that gives you access to generic macros and another, tied to the
SoC, that defines specific features.

## How to use features

DISCLAIMER: Generally speaking, it is wiser to write code independent from the
hardware features, because they will make your software behaviour dependent on a
feature. If you still need to do this, prefer doing this only at initialization
time.

If you want to check for a feature, you can simply include the `features.h` and
the `features_soc.h` headers.
You will be able to check if the board supports a feature by using the
`board_feature_has` macro, passing on argument one of the macros that is defined
in the `features_soc.h` file, that you will find in
`bsp/include/machine/soc/<manufacturer>/<soc>/features_soc.h`. Note that features are
initialized in the bootloader and they should be used as read-only in the
application.

For more information on the API see @ref features.

Example: The following code will check for the HW_OHRM_GPIO_SS feature.

\code
#include "infra/features.h"
#include "features_soc.h"

void f(void) {
	if (board_feature_has(HW_OHRM_GPIO_SS))
           use_these_gpios();
}
\endcode

