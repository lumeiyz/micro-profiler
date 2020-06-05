//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <frontend/listview.h>

#include "font_loader.h"

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/filling_rules.h>
#include <agge/figures.h>
#include <agge/stroke_features.h>
#include <agge.text/font.h>
#include <wpl/ui/layout.h>
#include <wpl/ui/scroller.h>

using namespace agge;
using namespace std;
using namespace placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		typedef blender_solid_color<simd::blender_solid_color, order_bgra> blender;

		class listview_complex : public layout_manager
		{
			virtual void layout(unsigned width, unsigned height, container::positioned_view *views, size_t count) const
			{
				const int scroller_width = 10;
				const int header_height = 20;
				const int height2 = height - header_height;

				// listview core
				if (count >= 1)
					views[0].location.left = 0, views[0].location.top = header_height, views[0].location.width = width, views[0].location.height = height2;

				// horizontal scroller
				if (count >= 3)
					views[1].location.left = 0, views[1].location.top = height - scroller_width, views[1].location.width = width, views[1].location.height = scroller_width;

				// vertical scroller
				if (count >= 2)
					views[2].location.left = width - scroller_width, views[2].location.top = header_height, views[2].location.width = scroller_width, views[2].location.height = height2;

				// header
				if (count >= 4)
					views[3].location.left = 0, views[3].location.top = 0, views[3].location.width = width, views[3].location.height = header_height;
			}
		};
	}


	column_header::column_header(text_engine_ptr text_engine_)
		: _text_engine(text_engine_)
	{	_font = _text_engine->create_font(L"Segoe UI", 11, true, false, agge::font::key::gf_vertical);	}

	void column_header::set_model(shared_ptr<columns_model> model)
	{
		_model = model;
		invalidate(0);
	}

	void column_header::draw(gcontext &ctx, gcontext::rasterizer_ptr &ras) const
	{
		typedef blender_solid_color<simd::blender_solid_color, order_bgra> blender;

		if (!_model)
			return;

		real_t x = 0.0f;
		const real_t y = _size.h - _font->get_metrics().descent;
		columns_model::column column;

		for (columns_model::index_type i = 0, n = _model->get_count(); i != n; ++i)
		{
			_model->get_column(i, column);
			_text_engine->render_string(*ras, *_font, column.caption.c_str(), layout::near, x, y,
				static_cast<real_t>(column.width));
			x += column.width;
		}
		ctx(ras, blender(color::make(0, 0, 0)), winding<>());
	}

	void column_header::resize(unsigned cx, unsigned cy, positioned_native_views &/*native_views*/)
	{	_size.w = static_cast<real_t>(cx), _size.h = static_cast<real_t>(cy);	}


	listview_core::listview_core(text_engine_ptr text_engine_, shared_ptr<column_header> cheader)
		: _text_engine(text_engine_), _cheader(cheader), _border_width(1.0f)
	{
		_font = _text_engine->create_font(L"Segoe UI", 11, false, false, agge::font::key::gf_vertical);
		agge::font::metrics m = _font->get_metrics();
		_item_height = real_t(int(1.4f * (m.leading + m.ascent + m.descent) + _border_width));
		_baseline_offset = real_t(int(0.5f * (_item_height + m.ascent - m.descent + _border_width)));

		_stroke.set_cap(agge::caps::butt());
		_stroke.set_join(agge::joins::bevel());
		_stroke.width(1.0f);
		_dash.add_dash(1.0f, 1.0f);
	}

	void listview_core::set_columns_model(shared_ptr<columns_model> cmodel)
	{
		_cheader->set_model(cmodel);
		controls::listview_core::set_columns_model(cmodel);
	}

	real_t listview_core::get_item_height() const
	{	return _item_height;	}

	void listview_core::draw_item_background(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b,
		index_type item, unsigned state) const
	{
		if (item)
		{
			add_path(*ras, rectangle(b.x1, b.y1, b.x2, b.y1 + _border_width));
			ctx(ras, blender(color::make(192, 192, 192)), winding<>());
		}
		add_path(*ras, rectangle(b.x1, b.y1 + _border_width, b.x2, b.y2));
		ctx(ras, blender(state & selected ? color::make(32, 208, 255)
			: item & 1 ? color::make(255, 255, 255) : color::make(224, 224, 224)), winding<>());
	}

	void listview_core::draw_item(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b, index_type /*item*/,
		unsigned state) const
	{
		if (state & focused)
		{
			add_path(*ras, assist(assist(rectangle(b.x1 + 0.25f, b.y1 + 0.5f, b.x2 - 0.25f, b.y2 - _border_width - 0.5f),
				_dash), _stroke));
			ctx(ras, blender(color::make(255, 255, 255)), winding<>());
		}
	}

	void listview_core::draw_subitem(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b, index_type /*item*/,
		unsigned /*state*/, index_type /*subitem*/, const wstring &text) const
	{
		_text_engine->render_string(*ras, *_font, text.c_str(), layout::near, b.x1, b.y1 + _baseline_offset, b.x2 - b.x1);
		ras->sort(true);
		ctx(ras, blender(color::make(0, 0, 0)), winding<>());
	}


	listview_controls create_listview()
	{
		shared_ptr<font_loader> l(new font_loader);
		shared_ptr<text_engine_t> e(new text_engine_t(*l, 4), [l] (text_engine_t *p) { delete p; });

		const shared_ptr<column_header> cheader(new column_header(e));
		const shared_ptr<listview_core> core(new listview_core(e, cheader));
		const shared_ptr<container> c(new container);
		const listview_controls result = { core, c };
		shared_ptr<scroller> vs(new scroller(scroller::vertical)), hs(new scroller(scroller::horizontal));

		vs->set_model(core->get_vscroll_model());
		hs->set_model(core->get_vscroll_model());
		c->set_layout(shared_ptr<layout_manager>(new listview_complex));
		c->add_view(core);
		c->add_view(hs);
		c->add_view(vs);
		c->add_view(cheader);
		return result;
	}
}