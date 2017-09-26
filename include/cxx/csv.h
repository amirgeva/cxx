#ifndef csv_h__
#define csv_h__

#include <vector>
#include <xstring.h>

class CSVReader
{
  typedef std::vector<xstring> str_vec;
  typedef std::vector<str_vec> rows_vec;
  str_vec m_Labels;
  rows_vec m_Rows;
  std::map<xstring, int> m_RowsIndex;
public:
  CSVReader(const xstring& filename, bool header_first_line)
  {
    std::ifstream fin(filename);
    if (fin.fail()) return;
    xstring line;
    if (header_first_line)
    {
      line.read_line(fin);
      xstring_tokenizer st(line, ",");
      while (st.has_more_tokens()) m_Labels.push_back(st.get_next_token());
    }
    while (line.read_line(fin))
    {
      m_Rows.push_back(str_vec());
      xstring_tokenizer st(line, ",");
      while (st.has_more_tokens()) m_Rows.back().push_back(st.get_next_token());
    }
  }

  void set_index_field(const xstring& label)
  {
    for (unsigned row = 0; row < size(); ++row)
    {
      xstring value=get(row, label);
      m_RowsIndex[value] = row;
    }
  }

  xstring operator() (int row, int col) const
  {
    return m_Rows[row].at(col);
  }

  xstring get(const xstring& row, const xstring& label) const
  {
    auto it = m_RowsIndex.find(row);
    if (it == m_RowsIndex.end()) return "";
    return get(it->second, label);
  }

  xstring get(int row, const xstring& label) const
  {
    auto it=std::find(m_Labels.begin(), m_Labels.end(), label);
    if (it == m_Labels.end()) return "";
    int col = std::distance(m_Labels.begin(), it);
    return m_Rows[row].at(col);
  }

  unsigned size() const { return m_Rows.size(); }
};

#endif // csv_h__
