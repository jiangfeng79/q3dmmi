#ifndef GEOTRANSFORM_H
#define GEOTRANSFORM_H

#include "layerGeometry.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Shared coordinate transforms.
//
// These convert between WGS84 (longitude/latitude), Web Mercator, and the map
// space the renderer expects. They are shared by the layer parsers (which
// build geometry) and the renderer (which projects to screen), so the math
// lives in one canonical place instead of being duplicated.
// ---------------------------------------------------------------------------

// Convert latitude (degrees) to Web Mercator Y (EPSG:4326 -> EPSG:900913).
inline double WGS84_TO_WGS84WEBMERCATOR(double Y)
{
    return log(tan((90.0 + Y) * M_PI / 360.0)) / (M_PI / 180.0);
}

// Convert Web Mercator Y back to latitude (degrees).
inline double WGS84WEBMERCATOR_TO_WGS84(double Y)
{
    return atan(pow(M_E, (Y) * M_PI / 180.0)) * 360.0 / M_PI - 90.0;
}

// Metre-space transform (default for most OSM layers).
inline float X_METRE_COORD_TO_MAP_COORD(double X, const MapProperty& p)
{
    return (float)((X) - p.centerX * p.mapBuildScale);
}

inline float Y_METRE_COORD_TO_MAP_COORD(double Y, const MapProperty& p)
{
    return (float)((Y) - p.centerY * p.mapBuildScale);
}

// WGS84 build transform (used by MAN_MADE / MRT layers).
inline float X_WGS84_BUILD_COORD_TO_MAP_COORD(double X, const MapProperty& p)
{
    return (float)((X) - p.centerX) * p.mapBuildScale;
}

inline float Y_WGS84_BUILD_COORD_TO_MAP_COORD(double Y, const MapProperty& p)
{
    return (float)(WGS84_TO_WGS84WEBMERCATOR(Y) - p.centerY) * p.mapBuildScale;
}

#endif // GEOTRANSFORM_H
