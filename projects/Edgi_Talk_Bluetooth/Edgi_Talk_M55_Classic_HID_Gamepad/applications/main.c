/* SPDX-License-Identifier: Apache-2.0 */
#include "classic_hid_host.h"
#include "gamepad.h"

int main(void)
{
    int rc = edgi_classic_gamepad_register();
    return rc ? rc : gamepad_demo_run("edgi-classic");
}
