
int CreateConvexHull(int width, int height, const unsigned char* pixels,
    int threshold, int sub_pixel, int max_vertex_count, float* vertices_xy);

int CreateConvexHullOptimize(int width, int height, const unsigned char* pixels,
    int threshold, int max_hull_size, int sub_pixel, int vertex_count, float* vertices_xy);
