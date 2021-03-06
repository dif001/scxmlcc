/*************************************************************************
 ** Copyright (C) 2014 Jan Pedersen <jp@jp-embedded.com>
 ** 
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 ** 
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** 
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

/*

   This is basically a simplified implementation of Andrei Alexandrescu's
   generalized functors. More information on this can be found in his
   book 'Modern C++'. This functor supports all C++ callable entities.

   The signal class is a functor itself, containing a vector of functors, 
   to invoke all clients when called.

*/

#ifndef __signal
#define __signal

#include <vector>
#include <memory>

struct null_t {};

template<class P0 = null_t> class functor_impl
{
	public:
	virtual functor_impl* clone() const = 0;
	virtual ~functor_impl() {}

	virtual void operator()(P0) = 0;
};

template<> class functor_impl<null_t>
{
	public:
	virtual functor_impl* clone() const = 0;
	virtual ~functor_impl() {}

	virtual void operator()() = 0;
};

// hander for functors and function pointers
template <class FN, class P0> class functor_handler : public functor_impl<P0>
{
	public:
	functor_handler(const FN& fn) : fun(fn) {}
	functor_handler* clone() const { return new functor_handler(*this); }

	void operator()() { fun(); }
	void operator()(P0 p0) { fun(p0); }
	
	private:
	FN fun;
};

// handler for member function
template <class O, class PMFN, class P0> class memfun_handler : public functor_impl<P0>
{
	public: 
	memfun_handler(const O& o, PMFN pm) : obj(o), pmfn(pm) {}
	memfun_handler* clone() const { return new memfun_handler(*this); }

	void operator()() { return ((*obj).*pmfn)(); }
	void operator()(P0 p0) { return ((*obj).*pmfn)(p0); }

	private:
	O obj;
	PMFN pmfn;
};

template<class P0 = null_t> class functor
{
	public:
	template <class FN> functor(const FN& fn) : impl(new functor_handler<FN, P0>(fn)) {};
	template <class O, class MFN> functor(const O& o, MFN mfn) : impl(new memfun_handler<O, MFN, P0>(o, mfn)) {};

#if __cplusplus > 199711L
	explicit functor(std::unique_ptr<functor_impl<P0> > f) : impl(std::move(f)) {}
#else
	explicit functor(std::auto_ptr<functor_impl<P0> > f) : impl(f) {}
#endif
	functor(const functor& f) : impl(f.impl->clone()) {}
	functor& operator =(const functor &f) {	impl.reset(f.impl->clone()); return *this; }

	void operator()() { return (*impl)(); }
	void operator()(P0 p0) { return (*impl)(p0); }

	private:
#if __cplusplus > 199711L
	std::unique_ptr<functor_impl<P0> > impl;
#else
	std::auto_ptr<functor_impl<P0> > impl;
#endif
};

template <class FN, class P0> class bind_handler : public functor_impl<>
{
	public:
	bind_handler(const FN& fn, P0 p0) : fun(fn), bound_p0(p0) {}
	bind_handler* clone() const { return new bind_handler(*this); }

	void operator()() { fun(bound_p0); }
	
	private:
	FN fun;
	P0 bound_p0;
};
#if __cplusplus > 199711L
template<class FN, class P0> functor<> bind(const FN &fn, P0 p0) { return functor<>(std::unique_ptr<functor_impl<> >(new bind_handler<FN, P0>(fn, p0))); }
#else
template<class FN, class P0> functor<> bind(const FN &fn, P0 p0) { return functor<>(std::auto_ptr<functor_impl<> >(new bind_handler<FN, P0>(fn, p0))); }
#endif

template<class P0 = null_t> class signal : public std::vector<functor<P0> > 
{
	public:
	template <class FN> void connect(const FN& fn) { this->push_back(fn); }
	template <class O, class MFN> void connect(const O& o, MFN mfn) { this->push_back(functor<P0>(o, mfn)); }

	void operator()() { for (typename impl::iterator i = impl::begin(); i != impl::end(); ++i) (*i)(); }
	void operator()(P0 p0) { for (typename impl::iterator i = impl::begin(); i != impl::end(); ++i) (*i)(p0); }
	
	private:
	typedef std::vector<functor<P0> > impl;
};

#endif
