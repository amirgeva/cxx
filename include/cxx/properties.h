#pragma once

#include <iostream>
#include <fstream>
#include <list>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <cxx/xstring.h>

namespace cxx {

typedef std::shared_ptr<class Properties> properties_ptr;

class Properties
{
	typedef std::unordered_map<xstring,xstring> str_map;
	typedef std::unordered_map<xstring, properties_ptr> child_map;
	str_map			m_Values;
	child_map		m_Children;
	
	void expand_macros(xstring& value)
	{
		bool done=false;
		while (!done)
		{
			int p=int(value.find("$("));
			if (p<0) done=true;
			else
			{
				int e=int(value.find(")",p+2));
				if (e<0) done=true;
				else
				{
					xstring key=value.substr(p+2,e-p-2);
					value=value.substr(0,p)+get(key)+value.substr(e+1);
				}
			}
		}
	}

	const xstring* get_internal(const xstring& name) const
	{
		int p=name.find(".");
		if (p>0)
		{
			auto it=m_Children.find(name.substr(0,p));
			if (it==m_Children.end()) return nullptr;
			return it->second->get_internal(name.substr(p+1));
		}
		const_iterator it=m_Values.find(name);
		if (it==m_Values.end()) return nullptr;
		return &(it->second);
	}
public:
	typedef str_map::const_iterator const_iterator;
	const_iterator begin() const { return m_Values.begin(); }
	const_iterator end()	 const { return m_Values.end(); }

	bool has(const xstring& name) const { return get_internal(name); }

	static properties_ptr load(const xstring& filename)
	{
		std::ifstream fin(filename);
		if (fin.fail()) return properties_ptr();
		return load(fin);
	}

	static properties_ptr load(std::istream& is)
	{
		auto props=std::make_shared<Properties>();
		xstring line;
		while (line.read_line(is))
		{
			line.trim();
			if (line.empty()) continue;
			if (line=="}") break;
			if (line.startswith("include "))
			{
				load(line.substr(8));
				continue;
			}
			if (line.endswith("{"))
			{
				xstring name=line.substr(0,line.length()-1);
				name.trim();
				auto child=load(is);
				props->m_Children[name]=child;				
			}
			int p=int(line.find('='));
			if (p<=0) continue;
			xstring name=line.substr(0,p);
			xstring value=line.substr(p+1);
			props->expand_macros(value);
			props->set(name,value);
		}
		return props;
	}

	bool save(std::ostream& os) const
	{
		for(const_iterator it=begin();it!=end();++it)
			os << it->first << "=" << it->second << std::endl;
		return true;
	}

	bool save(const xstring& filename) const
	{
		std::ofstream fout(filename);
		if (fout.fail()) return false;
		return save(fout);
	}

	void insert(const Properties& ps)
	{
		m_Values.insert(ps.begin(),ps.end());
	}

	template<class T>
	void set(const xstring& name, const T& value)
	{
		m_Values[name]=xstring(value);
	}

	const xstring& get(const xstring& name) const
	{
		static const xstring Null;
		auto ptr=get_internal(name);
		if (!ptr) return Null;
		return *ptr;
	}
	
	int getn(const xstring& name) const
	{
		return get(name).as_int();
	}

	template<class T>
	bool get(const xstring& name, T& value) const
	{
		auto ptr=get_internal(name);
		if (!ptr) return false;
		std::istringstream is(*ptr);
		is >> value;
		return !is.fail();
	}
	
	template<class T>
	void get(const xstring& name, T& value, const T& default_value) const
	{
		if (!get(name,value)) value=default_value;
	}
};

typedef std::list<Properties> prop_stack;

inline void push_properties(prop_stack& stack, const xstring& filename)
{
	if (stack.empty()) stack.push_back(Properties());
	else stack.push_back(stack.back());
	Properties ps;
	if (ps.load(filename))
		stack.back().insert(ps);
}

inline void pop_properties(prop_stack& stack)
{
	if (!stack.empty()) stack.pop_back();
}

} // namespace cxx

