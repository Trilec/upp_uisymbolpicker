#ifndef _Utilities_SymbolPicker_SymbolPickerModel_h_
#define _Utilities_SymbolPicker_SymbolPickerModel_h_

#include <Ui/Ui.h>

namespace Upp {

enum class SymbolPickerExportType : byte {
	ImageCall,
	IconId,
	CppSnippet,
	PngFiles,
	SvgFiles,
	UppRawHeader,
	UppRleHeader,
	UppIml,
	UppImlLibrary,
};

enum class SymbolPickerIconStyle : byte {
	Outlined,
	Rounded,
	Sharp,
};

struct SymbolPickerIconRef : Moveable<SymbolPickerIconRef> {
	String catalog_id;
	String source_id;
	String alias;
	int    size = 24;
	Color  tint = Null;
	String comment;
	String category_override;
	SymbolPickerIconStyle style_override = SymbolPickerIconStyle::Outlined;
	bool   has_style_override = false;
	bool   unresolved = false;
	bool   marked_used = false;
	String usage_note;
};

struct SymbolPickerCollection : Moveable<SymbolPickerCollection> {
	String                      name;
	String                      comment;
	String                      file_path;
	Vector<SymbolPickerIconRef> items;
	bool                        dirty = false;
};

struct SymbolPickerCustomAsset : Moveable<SymbolPickerCustomAsset> {
	String catalog_id;
	String file_path;
	String display_name;
};

struct SymbolPickerProject : Moveable<SymbolPickerProject> {
	String                        project_name;
	String                        comment;
	String                        file_path;
	String                        output_base_name;
	String                        symbol_prefix;
	bool                          shared_ui_catalog = false;
	SymbolPickerExportType        default_export_type = SymbolPickerExportType::ImageCall;
	Vector<SymbolPickerCustomAsset> custom_assets;
	int                           default_size = 48;
	Color                         default_tint = Null;
	SymbolPickerIconStyle         default_style = SymbolPickerIconStyle::Outlined;
	int default_optical_size = 20;
	bool default_filled = false;
	Vector<SymbolPickerCollection> collections;
	int                           active_collection_index = -1;
};

class SymbolPickerModel {
public:
	bool SetThemePreset(UiThemePreset preset);
	bool SetIconStyle(SymbolPickerIconStyle style);
	bool SetMaterialVariant(int optical_size, bool filled);
	bool SetCurrentCategory(const String& category);
	bool SetFilterText(const String& text);
	bool SetTintColor(Color color);
	bool SetExportType(SymbolPickerExportType type);
	bool SetExportSize(int px);
	bool AddIconToBin(const String& id);
	bool RemoveIconFromBin(const String& id);
	bool ClearBin();

	int  CreateCollection(const String& name, const String& file_path = String());
	bool RemoveCollection(int index);
	bool RenameCollection(int index, const String& name);
	bool SetActiveCollection(int index);
	bool AddIconToCollection(int collection_index, const SymbolPickerIconRef& ref);
	bool RemoveIconFromCollection(int collection_index, int item_index);
	bool MoveIconInCollection(int collection_index, int from_index, int to_index);
	bool ClearCollection(int collection_index);
	bool RenameCollectionIconAlias(int collection_index, int item_index, const String& alias);
	bool SetCollectionIconUsed(int collection_index, int item_index, bool used);
	bool SetProjectName(const String& name);
	bool SetProjectComment(const String& comment);
	bool SetProjectFilePath(const String& path);
	bool SetOutputBaseName(const String& name);
	bool SetSymbolPrefix(const String& prefix);
	bool AddCustomAsset(const SymbolPickerCustomAsset& asset);
	void MarkCollectionsSaved();
	SymbolPickerProject ExportProject() const;
	bool LoadProject(const SymbolPickerProject& project);

	UiThemePreset GetThemePreset() const { return theme_preset_; }
	SymbolPickerIconStyle GetIconStyle() const { return icon_style_; }
	int GetOpticalSize() const { return optical_size_; }
	bool GetFilled() const { return filled_; }
	const String& GetCurrentCategory() const { return current_category_; }
	const String& GetFilterText() const { return filter_text_; }
	Color GetTintColor() const { return tint_color_; }
	SymbolPickerExportType GetExportType() const { return export_type_; }
	int GetExportSize() const { return export_size_; }
	const Vector<String>& GetBinIconIds() const { return bin_icon_ids_; }
	const Vector<SymbolPickerCollection>& GetCollections() const { return collections_; }
	int GetActiveCollectionIndex() const { return active_collection_index_; }
	const String& GetProjectName() const { return project_name_; }
	const String& GetProjectComment() const { return project_comment_; }
	const String& GetProjectFilePath() const { return project_file_path_; }
	const String& GetOutputBaseName() const { return output_base_name_; }
	const String& GetSymbolPrefix() const { return symbol_prefix_; }
	const SymbolPickerCollection* GetActiveCollection() const;
	int FindBinIconIndex(const String& id) const;
	bool IsValidCollectionIndex(int index) const;
	bool IsValidItemIndex(int collection_index, int item_index) const;

	int GetRevision() const { return revision_; }
	int GetLibraryRevision() const { return library_revision_; }
	int GetCollectionsRevision() const { return collections_revision_; }
	int GetExportRevision() const { return export_revision_; }
	int GetProjectRevision() const { return project_revision_; }

	Event<> WhenChanged;

private:
	enum ChangeBits : dword {
		CHANGE_NONE = 0,
		CHANGE_LIBRARY = 1 << 0,
		CHANGE_COLLECTIONS = 1 << 1,
		CHANGE_EXPORT = 1 << 2,
		CHANGE_PROJECT = 1 << 3,
	};

	void Changed(dword bits = CHANGE_NONE);

	UiThemePreset theme_preset_ = UiThemePreset::Olive;
	SymbolPickerIconStyle icon_style_ = SymbolPickerIconStyle::Outlined;
	int optical_size_ = 20;
	bool filled_ = false;
	String current_category_ = "All";
	String filter_text_;
	Color tint_color_ = Null;
	SymbolPickerExportType export_type_ = SymbolPickerExportType::ImageCall;
	int export_size_ = 48;
	Vector<String> bin_icon_ids_;
	Vector<SymbolPickerCollection> collections_;
	int active_collection_index_ = -1;
	String project_name_;
	String project_comment_;
	String project_file_path_;
	String output_base_name_ = "symbols";
	String symbol_prefix_ = "ICON_";
	bool shared_ui_catalog_ = false;
	Vector<SymbolPickerCustomAsset> custom_assets_;

	int revision_ = 0;
	int library_revision_ = 0;
	int collections_revision_ = 0;
	int export_revision_ = 0;
	int project_revision_ = 0;
};

bool RunSymbolPickerModelRevisionSmokeTests(String& error);

}

#endif
