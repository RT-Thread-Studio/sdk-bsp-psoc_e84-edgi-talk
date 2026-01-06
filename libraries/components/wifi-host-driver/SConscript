Import('RTT_ROOT')
Import('rtconfig')
from building import *

src = []
path = []
cwd = GetCurrentDir()

# Define wifi host driver paths
whd = 'wifi-host-driver'
whd_path = cwd + '/' + whd
if GetDepend(['WHD_USING_WIFI6']):
    wifi_standard = 'COMPONENT_WIFI6'
else:
    wifi_standard = 'COMPONENT_WIFI5'
whd_src_path = whd_path + '/WHD' + '/' + wifi_standard
whd_src_path_prefix = whd + '/WHD' + '/' + wifi_standard

# Add the general drivers.
src += Glob(whd_src_path_prefix + '/src/bus_protocols/whd_bus_common.c')
src += Glob(whd_src_path_prefix + '/src/bus_protocols/whd_bus_sdio_protocol.c')
src += Glob(whd_src_path_prefix + '/src/bus_protocols/whd_bus.c')

# Add wifi host driver source and header files
src += Glob(whd_src_path_prefix + '/src/*.c')
path += [whd_src_path + '/src']
path += [whd_src_path + '/inc']
path += [whd_src_path + '/src/include']
path += [whd_src_path + '/src/bus_protocols']
path += [whd_src_path + '/resources/resource_imp']

# Add wifi host driver porting source and header files
src += Glob('porting/src/bsp/*.c')
src += Glob('porting/src/wlan/*.c')
src += Glob('porting/src/resources/*.c')

if GetDepend(['WHD_PORTING_BSP']):
    path += [cwd + '/porting/inc/bsp']

if GetDepend(['WHD_PORTING_HAL']):
    src += Glob('porting/src/hal/*.c')
    path += [cwd + '/porting/inc/hal']

if GetDepend(['WHD_PORTING_RTOS']):
    src += Glob('porting/src/rtos/*.c')
    path += [cwd + '/porting/inc/rtos']

# RT_USING_WIFI_HOST_DRIVER or PKG_USING_WIFI_HOST_DRIVER
group = DefineGroup('whd', src, depend = ['RT_USING_WIFI_HOST_DRIVER'], CPPPATH = path)
if GetDepend(['PKG_USING_WIFI_HOST_DRIVER']):
    group = DefineGroup('whd', src, depend = ['PKG_USING_WIFI_HOST_DRIVER'], CPPPATH = path)

Return('group')
