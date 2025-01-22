# KADAS Routing Plugin

Routing functionality for [KADAS Albireo 2](https://github.com/kadas-albireo/kadas-albireo2)

## Installation

This plugin is installed by default togheter with KADAS Albireo 2.
These softwares are needed to make the plugin works properly:

1. **KadasLocationSearch**, to enable name-based location search. It is usually shipped with Kadas. Without this, you can still choose a location using GPS or click on the map.
2. **Valhalla**, to enable routing, reachability, and navigation functionality. It must be installed on the same machine. Read more about Valhalla [here](https://github.com/valhalla/valhalla).

Currently, the installation only available from this repository.


## Development

### Updating data

Vehicles data used by the plugin can be updated by replacing the ``resources/vehicles.csv`` file, provided that the new file has the same table structure.

### Credit

- **Data Catalogue** icon is created on [LogoMakr.com](LogoMakr.com) with some modifications.
