// Shared Ui export preserves its public catalogue/list API.
static const char* kSymbolPickerUiCatalogAdapter = R"uiadapter(
using UiIconFactoryFn = Image (*)();

inline String UiIconDisplayName(const String& raw_name)
{
    String s = raw_name;
    if(s.StartsWith("ICON_"))
        s = s.Mid(5);
    if(s.EndsWith("_48"))
        s = s.Left(s.GetCount() - 3);
    s.Replace("_", " ");
    s = ToLower(s);

    String out;
    bool word_start = true;
    for(int i = 0; i < s.GetCount(); i++) {
        int c = s[i];
        if(IsSpace(c)) {
            out.Cat(' ');
            word_start = true;
        }
        else {
            out.Cat(word_start ? ToUpper(c) : c);
            word_start = false;
        }
    }
    return out;
}

struct UiIconCatalogEntry : Moveable<UiIconCatalogEntry> {
    String          name;
    String          display_name;
    UiIconFactoryFn factory = nullptr;

    UiIconCatalogEntry() {}
    UiIconCatalogEntry(const String& n, UiIconFactoryFn f) : name(n), display_name(UiIconDisplayName(n)), factory(f) {}
    UiIconCatalogEntry(const String& n, const String& d, UiIconFactoryFn f) : name(n), display_name(d), factory(f) {}
};

inline const Vector<UiIconCatalogEntry>& UiIconCatalog()
{
    static const Vector<UiIconCatalogEntry> catalog = [] {
        Vector<UiIconCatalogEntry> out;
        int count = 0;
        const auto* rows = GetUIICONSCatalog(count);
        out.Reserve(count);
        for(int i = 0; i < count; ++i)
            out.Add(UiIconCatalogEntry(rows[i].symbol_name, rows[i].factory));
        return out;
    }();
    return catalog;
}

inline Vector<String> UiIconNameList()
{
    Vector<String> names;
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    names.Reserve(catalog.GetCount());
    for(int i = 0; i < catalog.GetCount(); i++)
        names.Add(catalog[i].name);
    return names;
}

inline Vector<String> UiIconDisplayNameList()
{
    Vector<String> names;
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    names.Reserve(catalog.GetCount());
    for(int i = 0; i < catalog.GetCount(); i++)
        names.Add(catalog[i].display_name);
    return names;
}

inline UiListModel UiIconListModel(bool use_icon_name_as_data = true)
{
    UiListModel model;
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    model.Reserve(catalog.GetCount());
    for(int i = 0; i < catalog.GetCount(); i++) {
        UiModelItem it;
        it.text = catalog[i].display_name;
        if(use_icon_name_as_data)
            it.data = catalog[i].name;
        it.icon = catalog[i].factory ? catalog[i].factory() : Image();
        it.icon_render_mode = UiIconRenderMode::MonoTint;
        model.Add(it);
    }
    return model;
}

inline Image UiIconFromName(const String& icon_name)
{
    const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
    for(int i = 0; i < catalog.GetCount(); i++)
        if(icon_name == catalog[i].name)
            return catalog[i].factory ? catalog[i].factory() : Image();
    return Image();
}

)uiadapter";
