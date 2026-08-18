#include "SymbolPickerIconImageCache.h"

#include "SymbolPickerImageRender.h"

namespace Upp {

SymbolPickerIconImageCache::SymbolPickerIconImageCache()
{
}

void SymbolPickerIconImageCache::SetMaxEntries(int max_entries)
{
	max_entries_ = max(64, max_entries);
	if(images_.GetCount() > max_entries_)
		images_.Clear();
}

String SymbolPickerIconImageCache::MakeKey(const SymbolPickerIconEntry& entry, int pixel_size, Color tint) const
{
	return Format("%s|%d|%d|%d|%d",
		entry.catalog_id,
		max(1, pixel_size),
		IsNull(tint) ? -1 : tint.GetR(),
		IsNull(tint) ? -1 : tint.GetG(),
		IsNull(tint) ? -1 : tint.GetB());
}

Image SymbolPickerIconImageCache::RenderImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint) const
{
	return RenderSymbolPickerIconImage(entry, pixel_size, tint);
}

Image SymbolPickerIconImageCache::GetImage(const SymbolPickerIconEntry& entry, int pixel_size, Color tint)
{
	String key = MakeKey(entry, pixel_size, tint);
	int q = images_.Find(key);
	if(q >= 0) {
		++hit_count_;
		return images_[q];
	}

	++miss_count_;
	Image image = RenderImage(entry, pixel_size, tint);
	if(images_.GetCount() >= max_entries_)
		images_.Clear();
	images_.Add(key, image);
	return image;
}

void SymbolPickerIconImageCache::Clear()
{
	images_.Clear();
	hit_count_ = 0;
	miss_count_ = 0;
}

bool RunSymbolPickerIconImageCacheSmokeTests(const SymbolPickerCatalog& catalog, String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	if(catalog.GetIcons().IsEmpty())
		return Fail("Image-cache smoke test requires a non-empty catalog.");

	const SymbolPickerIconEntry* sample = nullptr;
	for(const auto& icon : catalog.GetIcons())
		if(icon.available) {
			sample = &icon;
			break;
		}
	if(!sample)
		return Fail("Image-cache smoke test could not find an available icon.");

	SymbolPickerIconImageCache cache;
	cache.SetMaxEntries(8192);
	Image first = cache.GetImage(*sample, 28, Black());
	if(first.IsEmpty())
		return Fail("Image-cache smoke test could not render sample icon.");
	Image second = cache.GetImage(*sample, 28, Black());
	if(second.IsEmpty() || cache.GetMissCount() != 1 || cache.GetHitCount() != 1 || cache.GetCount() != 1)
		return Fail("Image-cache smoke test did not reuse the rendered preview.");

	Image alternate = cache.GetImage(*sample, 32, Black());
	if(alternate.IsEmpty() || cache.GetMissCount() != 2 || cache.GetCount() != 2)
		return Fail("Image-cache smoke test did not separate preview-size keys.");

	cache.Clear();
	if(cache.GetCount() != 0 || cache.GetHitCount() != 0 || cache.GetMissCount() != 0)
		return Fail("Image-cache smoke test Clear() did not reset cache state.");

	error.Clear();
	return true;
}

}
