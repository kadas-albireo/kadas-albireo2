<!-- Recovered from: share/docs/html/en/en/gps/index.html -->
<!-- Language: en | Section: gps -->

# Navigation

The Navigation Tab groups functionalities for interacting with a connected GPS device, as well as tools for drawing, importing and exporting GPX (GPS Exchange Format) waypoints and routes.

## Enabling Geolocation

To use a GPS device with KADAS on Windows, the GpsGate Splitter application must be installed on the system, see [Configuring GPSGate](../gpsgate/gpsgate/).

The status of the GPS connection is displayed in the status bar in the lower area of the application window. This button can be toggled to set up resp. terminate a connection. The fill color of the status button changes depending on the current connection status

- **Black**: GPS disabled
- **Blue**: connection is being initialized
- **White**: connection initialized, but no data is being received
- **Red**: connection initialized, but no position data is available
- **Yellow**: connection initialized, only 2D fix
- **Green**: connection initialized, 3D fix

As soon as KADAS starts receiving position data from the GPS device, a marker will be placed on the map showing the current GPS position.

## Moving with Geolocation

This functions enables automatic centering of the visible map extent at the current GPS position.

## Drawing waypoints and routes

These functions allow drawing waypoints and routes, which can later be exported as GPS, for instance to upload to a GPS device.

**Waypoints** are simple points on the map, fitted with an optional name.

**Routes** are polylines, fitted with an option name and number.

Waypoints and routes are managed in a dedicated GPS Routes layer in the layer tree, analogous to redlining layers.

![](../media/image9.png)

## GPX export und import

These functions allow the user to export drawn waypoints and routed to a GPX file, as well as importing an existing GPX file into the **GPS Routes** layer.
