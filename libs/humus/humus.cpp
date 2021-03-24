
/* * * * * * * * * * * * * Author's note * * * * * * * * * * * *\
*   _       _   _       _   _       _   _       _     _ _ _ _   *
*  |_|     |_| |_|     |_| |_|_   _|_| |_|     |_|  _|_|_|_|_|  *
*  |_|_ _ _|_| |_|     |_| |_|_|_|_|_| |_|     |_| |_|_ _ _     *
*  |_|_|_|_|_| |_|     |_| |_| |_| |_| |_|     |_|   |_|_|_|_   *
*  |_|     |_| |_|_ _ _|_| |_|     |_| |_|_ _ _|_|  _ _ _ _|_|  *
*  |_|     |_|   |_|_|_|   |_|     |_|   |_|_|_|   |_|_|_|_|    *
*                                                               *
*                     http://www.humus.name                     *
*                                                                *
* This file is a part of the work done by Humus. You are free to   *
* use the code in any way you like, modified, unmodified or copied   *
* into your own work. However, I expect you to respect these points:  *
*  - If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  - For use in anything commercial, please request my approval.     *
*  - Share your work and ideas too as much as you can.             *
*                                                                *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "Vector.cpp"
#include "ConvexHull.cpp"
#include "ConvexHull.h"

typedef unsigned char ubyte;

namespace {
  ConvexHull CreateConvexHull(int w, int h, const ubyte* pixels, ubyte threshold, unsigned int max_hull_size, int sub_pixel)
  {
    ConvexHull hull;

    const float threshold_f = (float)threshold;
    const int ax = 0;
    const int ay = 0;
    const int tile_w = w;
    const int tile_h = h;

    const int start_x = ax * tile_w;
    const int start_y = ay * tile_h;
    const int end_x = start_x + tile_w - 1;
    const int end_y = start_y + tile_h - 1;

    const float off_x = 0.5f * (tile_w - 1);
    const float off_y = 0.5f * (tile_h - 1);
    const float corner_off_x = 0.5f * tile_w;
    const float corner_off_y = 0.5f * tile_h;

    // Corner cases
    if (pixels[start_y * w + start_x] > threshold)
    {
      hull.InsertPoint(float2(-corner_off_x, -corner_off_y));
    }

    if (pixels[start_y * w + end_x] > threshold)
    {
      hull.InsertPoint(float2(corner_off_x, -corner_off_y));
    }

    if (pixels[end_y * w + start_x] > threshold)
    {
      hull.InsertPoint(float2(-corner_off_x, corner_off_y));
    }

    if (pixels[end_y * w + end_x] > threshold)
    {
      hull.InsertPoint(float2(corner_off_x, corner_off_y));
    }

    // Edge cases
    const ubyte *row = pixels + start_y * w;
    for (int x = start_x; x < end_x; x++)
    {
      int c0 = row[x + 0];
      int c1 = row[x + 1];

      if ((c0 > threshold) != (c1 > threshold))
      {
        float d0 = (float) c0;
        float d1 = (float) c1;

        float sub_pixel_x = (threshold_f - d0) / (d1 - d0);
        hull.InsertPoint(float2(x - start_x - off_x + sub_pixel_x, -corner_off_y));
      }
    }

    row = pixels + end_y * w;
    for (int x = start_x; x < end_x; x++)
    {
      int c0 = row[x + 0];
      int c1 = row[x + 1];

      if ((c0 > threshold) != (c1 > threshold))
      {
        float d0 = (float) c0;
        float d1 = (float) c1;

        float sub_pixel_x = (threshold_f - d0) / (d1 - d0);
        hull.InsertPoint(float2(x - start_x - off_x + sub_pixel_x, corner_off_y));
      }
    }

    const ubyte *col = pixels + start_x;
    for (int y = start_y; y < end_y; y++)
    {
      int c0 = col[(y + 0) * w];
      int c1 = col[(y + 1) * w];

      if ((c0 > threshold) != (c1 > threshold))
      {
        float d0 = (float) c0;
        float d1 = (float) c1;

        float sub_pixel_y = (threshold_f - d0) / (d1 - d0);
        hull.InsertPoint(float2(-corner_off_x, y - start_y - off_y + sub_pixel_y));
      }
    }

    col = pixels + end_x;
    for (int y = start_y; y < end_y; y++)
    {
      int c0 = col[(y + 0) * w];
      int c1 = col[(y + 1) * w];

      if ((c0 > threshold) != (c1 > threshold))
      {
        float d0 = (float) c0;
        float d1 = (float) c1;

        float sub_pixel_y = (threshold_f - d0) / (d1 - d0);
        hull.InsertPoint(float2(corner_off_x, y - start_y - off_y + sub_pixel_y));
      }
    }




    // The interior pixels
    for (int y = start_y; y < end_y; y++)
    {
      const ubyte *row0 = pixels + (y + 0) * w;
      const ubyte *row1 = pixels + (y + 1) * w;

      for (int x = start_x; x < end_x; x++)
      {
        int c00 = row0[x + 0];
        int c01 = row0[x + 1];
        int c10 = row1[x + 0];
        int c11 = row1[x + 1];

        int count = 0;
        if (c00 > threshold) ++count;
        if (c01 > threshold) ++count;
        if (c10 > threshold) ++count;
        if (c11 > threshold) ++count;

        if (count > 0 && count < 4)
        {
          float d00 = (float) c00;
          float d01 = (float) c01;
          float d10 = (float) c10;
          float d11 = (float) c11;

          for (int n = 0; n <= sub_pixel; n++)
          {
            // Lerping factors
            float f0 = float(n) / float(sub_pixel);
            float f1 = 1.0f - f0;

            float x0 = d00 * f1 + d10 * f0;
            float x1 = d01 * f1 + d11 * f0;

            if ((x0 > threshold_f) != (x1 > threshold_f))
            {
              float sub_pixel_x = (threshold_f - x0) / (x1 - x0);
              hull.InsertPoint(float2(x - start_x - off_x + sub_pixel_x, y - start_y - off_y + f0));
            }

            float y0 = d00 * f1 + d01 * f0;
            float y1 = d10 * f1 + d11 * f0;

            if ((y0 > threshold_f) != (y1 > threshold_f))
            {
              float sub_pixel_y = (threshold_f - y0) / (y1 - y0);
              hull.InsertPoint(float2(x - start_x - off_x + f0, y - start_y - off_y + sub_pixel_y));
            }
          }
        }

      }

    }

    if (hull.GetCount() > max_hull_size)
    {
      do
      {
        if (!hull.RemoveLeastRelevantEdge())
        {
          break;
        }
      } while (hull.GetCount() > max_hull_size);
    }
    return hull;
  }
} // namespace

int CreateConvexHull(int width, int height, const unsigned char* pixels,
    int threshold, int sub_pixel, int max_vertex_count, float* vertices_xy)
{
  ConvexHull hull = CreateConvexHull(width, height, pixels, threshold, sub_pixel, max_vertex_count);

  hull.GoToFirst();
  float2 bias = float2(width, height) / 2.0f;
  for (int i = 0; ; i++)
  {
    vertices_xy[i * 2 + 0] = hull.GetCurrPoint().x + bias.x;
    vertices_xy[i * 2 + 1] = hull.GetCurrPoint().y + bias.y;
    if (!hull.GoToNext())
      break;
  }
  return hull.GetCount();
}

int CreateConvexHullOptimize(int width, int height, const unsigned char* pixels,
    int threshold, int max_hull_size, int sub_pixel, int vertex_count, float* vertices_xy)
{
  ConvexHull hull = CreateConvexHull(width, height, pixels, threshold, sub_pixel, max_hull_size);
  float2 polygon[8];
  float area;
  int count = hull.FindOptimalPolygon(polygon, vertex_count, &area);
  if (!count)
    return 0;

  // If fewer vertices were returned than asked for, just repeat the last vertex
  for (int i = count; i < vertex_count; i++)
  {
    polygon[i] = polygon[count - 1];
  }

  float2 bias = float2(width, height) / 2.0f;
  for (int i = 0; i < vertex_count; i++)
  {
    vertices_xy[i * 2 + 0] = polygon[i].x + bias.x;
    vertices_xy[i * 2 + 1] = polygon[i].y + bias.y;
  }
  return vertex_count;
}
