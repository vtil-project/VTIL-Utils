#pragma once

#include <vtil/arch>

struct pe_input
{
	vtil::pe_image& img;

	pe_input(vtil::pe_image& img)
		: img(img) {}

	bool is_valid(vtil::vip_t vip) const
	{
		auto last = img.get_section_count() - 1;
		auto ls = img.get_section(last);
		return vip >= img.get_image_base() && vip < img.get_image_base() + ls.virtual_size + ls.virtual_address;
	}

	uint8_t* get_at(vtil::vip_t vip) const
	{
		fassert(is_valid(vip));
		auto ptr = img.rva_to_ptr(vip - img.get_image_base());
		fassert(ptr != nullptr);
		return (uint8_t*)ptr;
	}
};