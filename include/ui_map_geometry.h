#ifndef UI_MAP_GEOMETRY_H
#define UI_MAP_GEOMETRY_H

// Keep map coordinates relative to the same right edge as the map texture.
static inline int UI_Map_ScaleX(int x, int rightEdge, int factor)
{
	return rightEdge + (x - rightEdge) * factor / 1000;
}

#endif
