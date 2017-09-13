#pragma once

#include <map>
#include <set>
#include <cxx/xml.h>

namespace cxx {

class Spreadsheet
{
  struct Cell
  {
    xstring value;
    xstring type;
    xstring text_color;
    xstring bg_color;

    bool default_style() const
    {
      return (text_color.empty() && bg_color.empty());
    }
  };

  struct Style
  {
    Style(const xstring& t="", const xstring& bg="")
      : text_color(t), bg_color(bg){}
    xstring text_color;
    xstring bg_color;
    xstring name;

    bool operator< (const Style& s) const
    {
      if (text_color == s.text_color) return bg_color < s.bg_color;
      return text_color < s.text_color;
    }
  };

  typedef std::set<Style> style_set;

  typedef std::map<int, Cell> Row;
  typedef std::map<int, Row> sheet;

  sheet m_Sheet;
  style_set m_Styles;

  xstring get_style(const xstring& text_color, const xstring& bg_color)
  {
    static int id = 61;
    Style s(text_color, bg_color);
    auto it = m_Styles.find(s);
    if (it == m_Styles.end())
    {
      s.name = "s" + xstring(++id);
      it=m_Styles.insert(s).first;
    }
    return it->name;
  }

  Cell& get(int row, int col)
  {
    auto it = m_Sheet.find(row);
    if (it == m_Sheet.end())
      it=m_Sheet.insert(std::make_pair(row,Row())).first;
    Row& r = it->second;
    auto cit = r.find(col);
    if (cit == r.end())
      cit = r.insert(std::make_pair(col, Cell())).first;
    return cit->second;
  }

  void generate_styles()
  {
    for (auto& row : m_Sheet)
    {
      for (auto it = row.second.begin(); it != row.second.end(); ++it)
      {
        auto& cell = it->second;
        if (!cell.default_style())
          get_style(cell.text_color, cell.bg_color);
      }
    }
  }
  static xstring hex(uint8_t v)
  {
    static const xstring h = "0123456789ABCDEF";
    xstring s = h.substr(v >> 4, 1) + h.substr(v & 15, 1);
    return s;
  }

public:
  static xstring rgb(uint8_t red, uint8_t green, uint8_t blue)
  {
    std::ostringstream os;
    os << "#" << hex(red) << hex(green) << hex(blue);
    return os.str();
  }

  void set_cell_value(int row, int col, double value)
  {
    auto& cell = get(row, col);
    cell.type = "Number";
    cell.value = xstring(value);
  }

  void set_cell_value(int row, int col, const xstring& text)
  {
    auto& cell = get(row, col);
    cell.type = "String";
    cell.value = text;
  }

  void set_cell_text_color(int row, int col, const xstring& color)
  {
    auto& cell = get(row, col);
    cell.text_color = color;
  }

  void set_cell_bg_color(int row, int col, const xstring& color)
  {
    auto& cell = get(row, col);
    cell.bg_color = color;
  }

  void generate(std::ostream& os)
  {
    generate_styles();
    xml_element book("Workbook");
    book.set_attribute("xmlns", "urn:schemas-microsoft-com:office:spreadsheet");
    book.set_attribute("xmlns:o", "urn:schemas-microsoft-com:office:office");
    book.set_attribute("xmlns:x", "urn:schemas-microsoft-com:office:excel");
    book.set_attribute("xmlns:ss", "urn:schemas-microsoft-com:office:spreadsheet");
    book.set_attribute("xmlns:html", "http://www.w3.org/TR/REC-html40");
    xml_ptr doc = book.add_child("DocumentProperties");
    doc->set_attribute("xmlns", "urn:schemas-microsoft-com:office:office");
    doc->add_child("Version")->set_content("15.00");
    xml_ptr docset=book.add_child("OfficeDocumentSettings");
    docset->set_attribute("xmlns", "urn:schemas-microsoft-com:office:office");
    docset->add_child("AllowPNG");
    xml_ptr wb = book.add_child("ExcelWorkbook");
    wb->set_attribute("xmlns", "urn:schemas-microsoft-com:office:excel");
    wb->add_child("ProtectStructure")->set_content("False");
    wb->add_child("ProtectWindows")->set_content("False");
    xml_ptr styles = book.add_child("Styles");
    xml_ptr defstyle = styles->add_child("Style");
    defstyle->set_attribute("ss:ID", "Default");
    defstyle->set_attribute("ss:Name", "Normal");
    defstyle->add_child("Alignment")->set_attribute("ss:Vertical", "Bottom");
    defstyle->add_child("Borders");
    xml_ptr deffont = defstyle->add_child("Font");
    deffont->set_attribute("ss:FontName", "Calibri");
    deffont->set_attribute("x:Family", "Swiss");
    deffont->set_attribute("ss:Size", "11");
    deffont->set_attribute("ss:Color", "#000000");
    defstyle->add_child("Interior");
    defstyle->add_child("NumberFormat");
    defstyle->add_child("Protection");
    for (auto& style : m_Styles)
    {
      xml_ptr st = styles->add_child("Style");
      st->set_attribute("ss:ID", style.name);
      if (!style.text_color.empty())
      {
        xml_ptr font=st->add_child("Font");
        font->set_attribute("ss:FontName", "Calibri");
        font->set_attribute("x:Family", "Swiss");
        font->set_attribute("ss:Size", "11");
        font->set_attribute("ss:Color", style.text_color);
      }
      if (!style.bg_color.empty())
      {
        xml_ptr bg = st->add_child("Interior");
        bg->set_attribute("ss:Color", style.bg_color); 
        bg->set_attribute("ss:Pattern", "Solid");
      }
    }
    xml_ptr sheet = book.add_child("Worksheet");
    sheet->set_attribute("ss:Name", "Sheet1");
    xml_ptr table = sheet->add_child("Table");
    table->set_attribute("ss:DefaultRowHeight", "15");

    for (int y = 1; y <= m_Sheet.rbegin()->first; ++y)
    {
      xml_ptr r = table->add_child("Row");
      r->set_attribute("ss:AutoFitHeight", "0");
      auto it = m_Sheet.find(y);
      if (it != m_Sheet.end())
      {
        Row& row = it->second;
        for (int x = 1; x <= row.rbegin()->first; ++x)
        {
          xml_ptr c = r->add_child("Cell");
          auto cit = row.find(x);
          if (cit != row.end())
          {
            Cell& cell = cit->second;
            if (!cell.default_style())
              c->set_attribute("ss:StyleID", get_style(cell.text_color, cell.bg_color));
            xml_ptr data=c->add_child("Data");
            data->set_attribute("ss:Type", cell.type);
            data->set_content(cell.value);
          }
        }
      }
    }
    xml_ptr opts=book.add_child("WorksheetOptions");
    opts->set_attribute("xmlns", "urn:schemas-microsoft-com:office:excel");
    xml_ptr page = opts->add_child("PageSetup");
    page->add_child("Header")->set_attribute("x:Margin", "0.3");
    page->add_child("Footer")->set_attribute("x:Margin", "0.3");
    page->add_child("PageMargins")->set_attribute("x:Bottom", "0.75")->set_attribute("x:Left", "0.7")
                                  ->set_attribute("x:Top", "0.75")->set_attribute("x:Right", "0.7");
    opts->add_child("Unsynced");
    xml_ptr prt = opts->add_child("Print");
    prt->add_child("ValidPrinterInfo");
    prt->add_child("HorizontalResolution")->set_content("600");
    prt->add_child("VerticalResolution")->set_content("600");
    opts->add_child("Selected");
    xml_ptr panes = opts->add_child("Panes");
    xml_ptr pane = panes->add_child("Pane");
    pane->add_child("Number")->set_content("3");
    pane->add_child("ActiveRow")->set_content("1");
    pane->add_child("ActiveCol")->set_content("1");
    opts->add_child("ProtectObjects")->set_content("False");
    opts->add_child("ProtectScenarios")->set_content("False");
    os << "<?xml version=\"1.0\"?>\n<?mso-application progid=\"Excel.Sheet\"?>\n";
    os << book.print(false);
  }
};

} // namespace cxx

