# KADAS Routing Plugin

Routing functionality for [KADAS Albireo 2](https://github.com/kadas-albireo/kadas-albireo2)

## Installation

These softwares are needed to make the plugin works properly:

1. **KadasLocationSearch**, to enable name-based location search. It is usually shipped with Kadas. Without this, you can still choose a location using GPS or click on the map.
2. **Valhalla**, to enable routing, reachability, and navigation functionality. It must be installed on the same machine. Read more about Valhalla [here](https://github.com/valhalla/valhalla).

Currently, the installation only available from this repository.

1. Obtaining the source code

    To get the source code, clone this repository:
	```
    git clone http://github.com/camptocamp/kadas-routing-plugin.git
    ```
    or download the source code using the [Github download link button](https://github.com/camptocamp/kadas-routing-plugin/archive/master.zip).

2. Installation

   To install this plugin on [Kadas Albireo 2](https://github.com/kadas-albireo/kadas-albireo2), extract the zip file from step 1, then copy the `kadasrouting` folder to the KADAS plugins folder located in your user folder. For example:
   ```
   C:\Users\fclementi\AppData\Roaming\Kadas\KadasMil\profiles\default\python\plugins
   ```

## Development

### Setup Development Environment

0. Install Kadas Albiero 2, currently, it is only available on Windows. The following steps are intended to run on a Windows machine.
1. Get the source code (see the [first step](##installation) in the installation).
2. Make a link of the `kadasrouting` to the Kadas's plugin directory by using the `mklink` command. The format is
   ```bash
   mklink /D TARGET_DIR SOURCE_DIR
   ```
   For example:
   ```bash
   mklink /D "C:\Users\fclementi\AppData\Roaming\Kadas\KadasMil\profiles\default\python\plugins\kadasrouting" "C:\Users\fclementi\Documents\GitHub\kadas-routing-plugin\kadasrouting"
   ```
3. You can also use Linux with Windows Virtual Machine. You need to mount the directory where the source code located on the Linux file system to the Windows Virtual Machine. Then do step 2 to make it available on Kadas Albiero 2.

   For example (using [Virtual Box](https://www.virtualbox.org/)):
   ```bash
   mklink /D "\\VBOXSVR\kadas-routing-plugin\kadasrouting" "C:\Users\fclementi\AppData\Roaming\Kadas\KadasMil\profiles\default\python\plugins\kadasrouting"
   ```

**Note**

The Kadas Albiero directory can be different. For example, if you use the portable version of Kadas, it may be in this path:

> C:\Users\fclementi\AppData\Roaming\Kadas\Kadas\profiles\default\python\plugins


### Internationalisation (i18n)

This plugin support 4 languages (DE, FR, IT, EN). The translation process is done on [this Transifex project](https://www.transifex.com/camptocamp/kadas-routing-plugin/dashboard/). Below are the commands that are used to manage the translation.

> It requires `docker`. See [here](https://docs.docker.com/get-docker/) to see how to install `docker`.

1. Updating string to be translated
    ```bash
    docker-update-translation-strings
    ```
2. Push string to be translated in Transifex
   ```bash
   make push-translation
   ```
3. Pull the translated string from Transifex
   ```bash
   make pull-translation
   ```
4. Compile translated string so that it is shown in the plugin
   ```bash
   make docker-compile-translation-strings
   ```
5. Extra: check the number of untranslated strings
    ```bash
    make docker-test-translation
    ```

See more about how to do the translation on this [wiki](https://github.com/camptocamp/kadas-routing-plugin/wiki/Internationalisation).

### Updating data

Vehicles data used by the plugin can be updated by replacing the ``resources/vehicles.csv`` file, provided that the new file has the same table structure.

### Credit

- **Data Catalogue** icon is created on [LogoMakr.com](LogoMakr.com) with some modifications.
