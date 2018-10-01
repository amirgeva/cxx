/***************************************************************************
Copyright (c) 2013-2018, Amir Geva
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once

#include <algorithm>
#include <list>
#include <opencv2/opencv.hpp>
#include <cxx/prims.h>
#include <cxx/xstring.h>

namespace cxx {
	
  namespace cc {

    typedef unsigned char byte;
    typedef std::pair<unsigned, unsigned> pixel_coords;
    typedef std::vector<pixel_coords> coords_vec;
    using cv::Rect;

    Rect& unite(cv::Rect& r, int px, int py)
    {
      if (px < r.x)
      {
        r.width += r.x - px;
        r.x = px;
      }
      if (py < r.y)
      {
        r.height += r.y - py;
        r.y = py;
      }
      if (px >= (r.x + r.width))
      {
        r.width = px - r.x + 1;
      }
      if (py >= (r.y + r.height))
      {
        r.height = py - r.y + 1;
      }
      return r;
    }

    Rect& unite(Rect& r, const Rect& other)
    {
      int x0 = other.x, y0 = other.y, x1 = x0 + other.width - 1, y1 = y0 + other.height - 1;
      unite(r, x0, y0);
      unite(r, x0, y1);
      unite(r, x1, y0);
      unite(r, x1, y1);
      return r;
    }

    bool valid_rect(const Rect& r) 
    { 
      return r.width > 0 && r.height > 0; 
    }

    // Connected component to be used in the connected components search algorithm
    struct cc
    {
      cc() : leader(0), pixels(0), cgx(0), cgy(0) {}
      cc*   leader;  // Internally used.  Leader for the equivalence class
      int   pixels;  // number of pixels
      float cgx, cgy; // Component's center of gravity
      Rect  rect;    // Bounding rectangle
      coords_vec coords;

      void follow()
      {
        while (leader && leader->leader)
          leader = leader->leader;
      }

      void verify()
      {
        //return;
        for (const auto& p : coords)
        {
          int x = p.first, y = p.second;
          if (x < rect.x || y < rect.y || x >= (rect.x + rect.width) || y >= (rect.y + rect.height))
          {
            throw xstring("Verify failed.");
          }
        }
      }

      // Add a new pixel at position
      void add(unsigned x, unsigned y, bool save_coords)
      {
        follow();
        if (leader) leader->add(x, y, save_coords);
        else
        {
          cgx = (cgx*pixels + x) / float(pixels + 1);
          cgy = (cgy*pixels + y) / float(pixels + 1);
          pixels++;
          if (rect.width == 0) rect = Rect(x, y, 1, 1);
          else cxx::cc::unite(rect,x, y);
          if (save_coords)
            coords.push_back(std::make_pair(x, y));
        }
        verify();
      }

      // Unite 2 cc's when a pixel bridges between them
      bool unite(cc& c)
      {
        if (&c == this) return true;
        follow();
        if (leader) return leader->unite(c);
        if (c.leader) return unite(*c.leader);
        if (c.pixels > pixels) return c.unite(*this);
        cgx = (cgx*pixels + c.cgx*c.pixels) / float(pixels + c.pixels);
        cgy = (cgy*pixels + c.cgy*c.pixels) / float(pixels + c.pixels);
        pixels += c.pixels;
        cxx::cc::unite(rect,c.rect);
        c.leader = this;
        coords.insert(coords.end(), c.coords.begin(), c.coords.end());
        verify();
        return true;
      }

      bool has_leader() const { return leader != 0; }

      cv::Mat build_image() const
      {
        cv::Mat image(rect.height, rect.width, CV_8UC1);
        image = cv::Scalar(0, 0, 0, 255);
        coords_vec::const_iterator b = coords.begin(), e = coords.end();
        for (; b != e; ++b)
        {
          const pixel_coords& p = *b;
          unsigned y = p.second - rect.y, x = p.first - rect.x;
          uint8_t* row = image.ptr(y);
          row[x] = 255;
        }
        return image;
      }

      double get_density() const
      {
        return double(pixels) / rect.area();
      }

      //typedef std::list<cc> cc_list;

    };

    typedef std::list<cc> cc_list;
    typedef std::vector<cc> cc_vec;

    struct config
    {
      config()
        : min_x(0), min_y(0), max_x(9999), max_y(9999),
        active_pixel(0),
        bin_thres(1),
        collect_images(false)
      {}
      unsigned min_x, max_x;
      unsigned min_y, max_y;
      bool     collect_images;
      byte     active_pixel;
      byte     bin_thres;
    };

    // Main scanning algorithm.   Builds equivalence classes for components
    void analyze(const cv::Mat& image, cc_list& l, const config& cfg = config())
    {
      static cc* const null_cc = 0;
      typedef std::vector<cc*> ccp_vec;
      ccp_vec last(image.cols, 0), cur(image.cols, 0);
      l.clear();
      for (unsigned y = cfg.min_y; y < unsigned(image.rows) && y < cfg.max_y; ++y)
      {
        const byte* row = image.ptr(y);
        for (unsigned x = cfg.min_x; x < unsigned(image.cols) && x < cfg.max_x; ++x)
        {
          byte pixel = row[x];
          if (pixel == cfg.active_pixel || pixel < cfg.bin_thres)
          {
            if (last[x])
            {
              last[x]->add(x, y, cfg.collect_images);
              cur[x] = last[x];
              if (x > 0 && cur[x - 1]) last[x]->unite(*cur[x - 1]);
            }
            else
            if (x > 0 && cur[x - 1])
            {
              cur[x - 1]->add(x, y, cfg.collect_images);
              cur[x] = cur[x - 1];
            }
            else
            {
              l.push_back(cc());
              l.back().add(x, y, cfg.collect_images);
              cur[x] = &(l.back());
            }
          }
        }
        last.swap(cur);
        std::fill(cur.begin(), cur.end(), null_cc);
      }
      // Remove all the zombies that are left leaderless
      l.erase(std::remove_if(l.begin(), l.end(),
                             [](const cc& c)->bool {return c.has_leader(); }),
              l.end());
    }

    struct smaller_than : public std::unary_function < cc, bool >
    {
      int N;
      smaller_than(int n) : N(n) {}
      bool operator() (const cc& c) const { return c.pixels < N; }
    };

    struct shorter_than : public std::unary_function < cc, bool >
    {
      int N;
      shorter_than(int n) : N(n) {}
      bool operator() (const cc& c) const { return c.rect.height < N; }
    };

    inline void remove_small_cc(cc_list& l, int min_size)
    {
      l.erase(std::remove_if(l.begin(), l.end(), smaller_than(min_size)), l.end());
    }

    inline void remove_short_cc(cc_list& l, int min_size)
    {
      l.erase(std::remove_if(l.begin(), l.end(), shorter_than(min_size)), l.end());
    }

    typedef cv::Point2d Vec2D;
    typedef cv::Point_<short> sPoint;

    class icc_group
    {
      Rect     m_Rect;
      unsigned m_Pixels;
      Vec2D    m_CG;
    public:
      icc_group() : m_Rect(0, 0, 0, 0), m_Pixels(0), m_CG(0, 0) {}

      unsigned size() const { return m_Pixels; }
      const Rect& get_rect() const { return m_Rect; }
      Vec2D get_cg() const
      {
        if (m_Pixels == 0) return m_CG;
        Vec2D res = m_CG;
        res *= (1.0 / m_Pixels);
        return res;
      }

      void reset()
      {
        m_Rect = Rect(0, 0, 0, 0);
        m_Pixels = 0;
      }

      void add(const sPoint& coords)
      {
        if (!valid_rect(m_Rect)) m_Rect = Rect(coords.x, coords.y, 1, 1);
        else                 cxx::cc::unite(m_Rect,coords.x, coords.y);
        m_CG += Vec2D(coords.x, coords.y);
        ++m_Pixels;
      }

      void join(icc_group* other)
      {
        cxx::cc::unite(m_Rect,other->m_Rect);
        m_Pixels += other->size();
        m_CG += other->m_CG;
        other->reset();
      }
    };

    class icc_pixel
    {
      sPoint     m_Coords;
      icc_pixel* m_Leader;
      icc_group* m_Group;
      byte       m_Intensity;
    public:
      icc_pixel(ushort x, ushort y, byte i) : m_Coords(x, y), m_Leader(0), m_Group(0), m_Intensity(i)
      {
      }

      byte get_intensity() const { return m_Intensity; }

      const sPoint& get_coords() const { return m_Coords; }

      icc_pixel* update_leader()
      {
        while (m_Leader != m_Leader->m_Leader)
          m_Leader = m_Leader->m_Leader;
        return m_Leader;
      }

      void add_to_group(icc_pixel& pix)
      {
        if (!m_Group) return;
        m_Group->add(pix.get_coords());
      }

      void join(icc_pixel* other)
      {
        if (other == this) return;
        if (m_Group->size() < other->m_Group->size())
        {
          other->join(this);
          return;
        }
        icc_group* other_group = other->m_Group;
        other->m_Leader = update_leader();
        m_Group->join(other_group);
        other->m_Group = 0;
      }

      icc_group* attach(icc_pixel& pix)
      {
        icc_group* res = 0;
        if (m_Leader)
        {
          if (pix.m_Leader)
          {
            update_leader()->join(pix.update_leader());
          }
          else
          {
            pix.m_Leader = update_leader();
            m_Leader->add_to_group(pix);
          }
        }
        else
        {
          if (pix.m_Leader)
          {
            m_Leader = pix.update_leader();
            m_Leader->add_to_group(*this);
          }
          else
          {
            m_Leader = this;
            pix.m_Leader = this;
            res = m_Group = new icc_group;
            add_to_group(*this);
            add_to_group(pix);
          }
        }
        return res;
      }
    };

  } // namespace cc

} // namespace cxx


