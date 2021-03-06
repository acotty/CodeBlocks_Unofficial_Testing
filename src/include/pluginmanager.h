/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 */

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <vector>
#include <map>
#include <set>

#include <wx/dynarray.h>
#include "globals.h" // PluginType
#include "settings.h"
#include "manager.h"

//forward decls
struct PluginInfo;
class cbPlugin;
class cbCompilerPlugin;
class cbMimePlugin;
class cbConfigurationPanel;
class cbConfigurationPanelColoursInterface;
class cbProject;
class wxDynamicLibrary;
class wxMenuBar;
class wxMenu;
class CodeBlocksEvent;
class TiXmlDocument;
class FileTreeData;

// typedefs for plugins' function pointers
typedef void(*PluginSDKVersionProc)(int *, int *, int *);
typedef cbPlugin * (*CreatePluginProc)();
typedef void(*FreePluginProc)(cbPlugin *);

/** Information about the plugin */
struct PluginInfo
{
    wxString name;
    wxString title;
    wxString version;
    wxString description;
    wxString author;
    wxString authorEmail;
    wxString authorWebsite;
    wxString thanksTo;
    wxString license;
};

// struct with info about each pluing
struct PluginElement
{
    PluginInfo info; // plugin's info struct
    wxString fileName; // plugin's filename
    wxDynamicLibrary * library; // plugin's library
    FreePluginProc freeProc; // plugin's release function pointer
    cbPlugin * plugin; // the plugin itself
};

WX_DEFINE_ARRAY(PluginElement *, PluginElementsArray);
WX_DEFINE_ARRAY(cbPlugin *, PluginsArray);
WX_DEFINE_ARRAY(cbConfigurationPanel *, ConfigurationPanelsArray);

/**
 * PluginManager manages plugins.
 *
 * There are two plugin types: binary and scripted.
 *
 * Binary plugins are dynamically loaded shared libraries (dll/so) which
 * can do pretty much anything with the SDK.
 *
 * Script plugins are more lightweight and are very convenient for
 * smaller scale/functionality plugins.
 */
class DLLIMPORT PluginManager : public Mgr<PluginManager>, public wxEvtHandler
{
    public:
        typedef std::vector<cbCompilerPlugin *> CompilerPlugins;

    public:
        friend class Mgr<PluginManager>;
        friend class Manager; // give Manager access to our private members
        void CreateMenu(wxMenuBar * menuBar);
        void ReleaseMenu(wxMenuBar * menuBar);

        void RegisterPlugin(const wxString & name,
                            CreatePluginProc createProc,
                            FreePluginProc freeProc,
                            PluginSDKVersionProc versionProc);

        int ScanForPlugins(const wxString & path);
        bool LoadPlugin(const wxString & pluginName);
        void LoadAllPlugins();
        void UnloadAllPlugins();
        void UnloadPlugin(cbPlugin * plugin);
        int ExecutePlugin(const wxString & pluginName);

        bool AttachPlugin(cbPlugin * plugin, bool ignoreSafeMode = false);
        bool DetachPlugin(cbPlugin * plugin);

        bool InstallPlugin(const wxString & pluginName, bool forAllUsers = true, bool askForConfirmation = true);
        bool UninstallPlugin(cbPlugin * plugin, bool removeFiles = true);
        bool ExportPlugin(cbPlugin * plugin, const wxString & filename);

        const PluginInfo * GetPluginInfo(const wxString & pluginName);
        const PluginInfo * GetPluginInfo(cbPlugin * plugin);

        const PluginElementsArray & GetPlugins() const
        {
            return m_Plugins;
        }

        PluginElement * FindElementByName(const wxString & pluginName);
        cbPlugin * FindPluginByName(const wxString & pluginName);
        cbPlugin * FindPluginByFileName(const wxString & pluginFileName);

        const CompilerPlugins & GetCompilerPlugins() const
        {
            return m_CompilerPlugins;
        }
        cbCompilerPlugin * GetFirstCompiler() const;

        PluginsArray GetToolOffers();
        PluginsArray GetMimeOffers();
        PluginsArray GetDebuggerOffers();
        PluginsArray GetCodeCompletionOffers();
        PluginsArray GetSmartIndentOffers();
        PluginsArray GetOffersFor(PluginType type);

        void AskPluginsForModuleMenu(const ModuleType type, wxMenu * menu, const FileTreeData * data = nullptr);
        /// Called by the code creating the context menu for the editor. Must not be called by
        /// plugins.
        void ResetModuleMenu();
        /// Can be called by plugins' BuildModuleMenu when building the EditorManager's context
        /// menu. This method has two purposes:
        /// 1. to control the number of find related items in the menu
        /// 2. to control the number of items that are placed before the find related items.
        /// @param before Pass true when you're adding items before the find group and false when
        /// adding in the find group.
        /// @param count The number of items you're adding to the menu.
        void RegisterFindMenuItems(bool before, int count);
        /// Returns the number of items in the find group already added to the menu.
        int GetFindMenuItemCount() const;
        /// Returns the position of the first menu item in the find group.
        int GetFindMenuItemFirst() const;
        /// Called by the editor code which adds the non-plugin related menu items to store the id
        /// of the last fixed menu item. Must not be called by plugins.
        void RegisterLastNonPluginMenuItem(int id);
        /// Called by plugins when they want to add menu items to the editor's context menu.
        /// Using this method will produce a menu which is sorted alphabetically (case
        /// insensitive). The menu items are added at the bottom of the menu.
        /// @param popup The context menu passed to BuildModuleMenu.
        /// @param label The label of the new menu item. It will be used to find the correct
        /// position.
        /// @return The position where to insert the item.
        int FindSortedMenuItemPosition(wxMenu & popup, const wxString & label) const;

        cbMimePlugin * GetMIMEHandlerForFile(const wxString & filename);
        void GetConfigurationPanels(int group, wxWindow * parent,
                                    ConfigurationPanelsArray & arrayToFill,
                                    cbConfigurationPanelColoursInterface * coloursInterface);
        void GetProjectConfigurationPanels(wxWindow * parent, cbProject * project, ConfigurationPanelsArray & arrayToFill);
        int Configure();
        void SetupLocaleDomain(const wxString & DomainName);

        void NotifyPlugins(CodeBlocksEvent & event);
        void NotifyPlugins(CodeBlocksDockEvent & event);
        void NotifyPlugins(CodeBlocksLayoutEvent & event);

        static void SetSafeMode(bool on)
        {
            s_SafeMode = on;
        }
        static bool GetSafeMode()
        {
            return s_SafeMode;
        }
    private:
        PluginManager();
        ~PluginManager() override;

        void OnScriptMenu(wxCommandEvent & event);
        void OnScriptModuleMenu(wxCommandEvent & event);

        /// @return True if the plugin should be loaded, false if not.
        bool ReadManifestFile(const wxString & pluginFilename,
                              const wxString & pluginName = wxEmptyString,
                              PluginInfo * infoOut = nullptr);
        void ReadExtraFilesFromManifestFile(const wxString & pluginFilename,
                                            wxArrayString & extraFiles);
        bool ExtractFile(const wxString & bundlename,
                         const wxString & src_filename,
                         const wxString & dst_filename,
                         bool isMandatory = true);

        PluginElementsArray m_Plugins;
        wxString m_CurrentlyLoadingFilename;
        wxDynamicLibrary * m_pCurrentlyLoadingLib;
        TiXmlDocument * m_pCurrentlyLoadingManifestDoc;

        // this struct fills the following vector each time
        // RegisterPlugin() is called.
        // this vector is then used in LoadPlugin() (which triggered
        // the call to RegisterPlugin()) to actually
        // load the plugins and then it is cleared.
        //
        // This is done to avoid global variables initialization order issues
        // inside the plugins (yes, it happened to me ;)).
        struct PluginRegistration
        {
            PluginRegistration() : createProc(nullptr), freeProc(nullptr), versionProc(nullptr) {}
            PluginRegistration(const PluginRegistration & rhs)
                : name(rhs.name),
                  createProc(rhs.createProc),
                  freeProc(rhs.freeProc),
                  versionProc(rhs.versionProc),
                  info(rhs.info)
            {}
            wxString name;
            CreatePluginProc createProc;
            FreePluginProc freeProc;
            PluginSDKVersionProc versionProc;
            PluginInfo info;
        };
        std::vector<PluginRegistration> m_RegisteredPlugins;
        CompilerPlugins m_CompilerPlugins;

        int m_FindMenuItemCount = 0;
        int m_FindMenuItemFirst = 0;
        int m_LastNonPluginMenuId = 0;

        static bool s_SafeMode;

        DECLARE_EVENT_TABLE()
};

DLLIMPORT bool cbHasRunningCompilers(const PluginManager * manager);
DLLIMPORT void cbStopRunningCompilers(PluginManager * manager);
DLLIMPORT void cbUpdateCompilersSetupEnvironment(PluginManager * manager);

#endif // PLUGINMANAGER_H
