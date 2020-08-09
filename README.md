# KADAS Routing Plugin

Routing functionality for [KADAS Albireo 2](https://github.com/kadas-albireo/kadas-albireo2)

## Installation

Currently, the installation only available from this repository.

1. Obtaining the source code

    To get the source code, clone this repository:
	```
    git clone http://github.com/camptocamp/kadas-routing-plugin.git
    ```
    or download the source code using the [Github download link button](https://github.com/camptocamp/kadas-routing-plugin/archive/master.zip).

2. Installation

   To install this plugin on [Kadas Albireo 2](https://github.com/kadas-albireo/kadas-albireo2), copy the copy the `kadasrouting` folder to the KADAS plugins folder located in your used folder. For example:
   ```
   C:\Users\fclementi\AppData\Roaming\Kadas\KadasMil\profiles\default\python\plugins
   ```

## Development

### Setup Development Environment

0. Install Kadas Albiero 2, currently it is only available on Windows. The following steps are intended to run on Windows machine.
1. Get the source code (see the [first step](##installation) in the installation).
2. Make a link of the `kadasrouting` to the Kadas's plugin directory, for example:
   ```
   mklink /D "C:\Users\fclementi\AppData\Roaming\Kadas\KadasMil\profiles\default\python\plugins\kadasrouting" "C:\Users\fclementi\Documents\GitHub\kadas-routing-plugin\kadasrouting"
   ```
3. You can also use Linux with Windows Virtual Machine. You need to mount the directory where the source code located on the Linux file system to the Windows Virtual Machine. Then do step 2 to make it available on Kadas Albiero 2.

### Internationalisation (i18n)

This plugin support 4 languages (DE, FR, IT, EN). See more about how to do translation on this [wiki](https://github.com/camptocamp/kadas-routing-plugin/wiki/Internationalisation).