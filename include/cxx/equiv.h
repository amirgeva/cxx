#pragma once

#include <vector>
#include <memory>

namespace cxx {

typedef std::vector<int> int_vec;

class Leader;
typedef std::shared_ptr<Leader> leader_ptr;

class Leader
{
  int_vec members;
public:
  void add(int i)
  {
    members.push_back(i);
  }

  int size() const { return int(members.size()); }

  void unite(leader_ptr lj)
  {
    members.insert(members.end(),lj->members.begin(),lj->members.end());
  }

  const int_vec& get_members() const { return members; }
};

template<class T>
class EquivalenceItem
{
  leader_ptr m_Leader;
  T          m_Data;
public:
  EquivalenceItem() : m_Leader(0) {}
  EquivalenceItem(const T& data) : m_Leader((Leader*)0), m_Data(data) {}
  const T& get_data() const { return m_Data; }
  void set_data(const T& data) { m_Data=data; }
  void set_leader(leader_ptr leader) 
  { 
    m_Leader=leader; 
  }
  leader_ptr get_leader() const { return m_Leader; }
};

template<class T>
class EquivalenceClassifier
{
  typedef EquivalenceClassifier<T> self;
  typedef EquivalenceItem<T> item_type;
  typedef std::vector<item_type> items_vec;

  items_vec m_Items;

  /*
  int get_leader(int i)
  {
    int leader=m_Items[i].get_leader();
    if (leader<0) return i;
    leader=get_leader(leader);
    m_Items[i].set_leader(leader);
    return leader;
  }
  */

  class GroupIterator : public std::iterator<std::forward_iterator_tag,int_vec>
  {
    typedef typename items_vec::const_iterator base_iterator;
    base_iterator m_Iter,m_End;
    int           m_Index;
    
    bool is_valid() const
    {
      leader_ptr l=m_Iter->get_leader();
      if (!l) return false;
      return (l->get_members().front() == m_Index);
    }
  public:
    GroupIterator(base_iterator begin, base_iterator end) 
      : m_Iter(begin), m_End(end), m_Index(0)
    {
      if (m_Iter!=m_End)
      {
        if (!is_valid()) ++(*this);
      }
    }
    const int_vec& operator*  () const { return m_Iter->get_leader()->get_members(); }
    const int_vec* operator-> () const { return &(m_Iter->get_leader()->get_members()); }
    GroupIterator& operator++ () 
    {
      while (++m_Iter!=m_End)
      {
        ++m_Index;
        if (is_valid()) break;
      }
      return *this;
    }
    GroupIterator operator++ (int) { GroupIterator it=*this; ++(*this); return it; }
    bool operator== (const GroupIterator& rhs) const { return m_Iter==rhs.m_Iter; }
    bool operator!= (const GroupIterator& rhs) const { return !(*this==rhs); }
  };
public:
  const T& get(int i) const { return m_Items[i].get_data(); }
  const T& operator[] (int i) const { return get(i); }
  int  size() const { return int(m_Items.size()); }
  int add_item(const T& item) 
  { 
    int i= int(m_Items.size());
    m_Items.push_back(item_type(item)); 
    leader_ptr li(new Leader);
    m_Items.back().set_leader(li);
    li->add(i);
    return i;
  }

  template<class II>
  void add_items(II b, II e)
  {
    for (; b != e; ++b)
      add_item(*b);
  }

  template<class PRED>
  void add_equivalences(PRED p)
  {
    for (int i = 0; i < size();++i)
    {
      for (int j = i+1; j < size(); ++j)
      {
        if (p(get(i), get(j)))
          add_equivalence(i, j);
      }
    }
  }

  void add_equivalence(int i, int j)
  {
    if (i<0 || i>=size() || j<0 || j>=size() || i==j) return;
    leader_ptr li=m_Items[i].get_leader();
    leader_ptr lj=m_Items[j].get_leader();
    if (!li || !lj)
    {
      if (!li && !lj)
      {
        li=leader_ptr(new Leader);
        m_Items[i].set_leader(li);
        li->add(i);
      }
      if (li) 
      {
        li->add(j);
        m_Items[j].set_leader(li);
      }
      else
      {
        lj->add(i);
        m_Items[i].set_leader(lj);
      }
    }
    else
    {
      // Both i and j have leaders
      if (li!=lj)
      {
        if (li->size() < lj->size())
        {
          std::swap(i,j);
          std::swap(li,lj);
        }
        li->unite(lj);
        const int_vec& v=lj->get_members();
        for(int_vec::const_iterator b=v.begin();b!=v.end();++b)
          m_Items[*b].set_leader(li);
      }
    }
  }

  typedef GroupIterator group_iterator;
  group_iterator begin() { return group_iterator(m_Items.begin(),m_Items.end()); }
  group_iterator end()   { return group_iterator(m_Items.end(),m_Items.end()); }

};

} // namespace cxx

