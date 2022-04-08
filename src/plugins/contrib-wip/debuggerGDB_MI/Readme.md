# CODE::BLOCKS  GDB/MI DEBUGGER PLUGIN

<font size="4" style="color:red">**NOTE: As of 05-Apr-2022 there C::B source code changes needed in order to build the plugin and these have not been submitted, so you will not be able to build the debugger at the moment!!**
</font>

## Description

This GitHub repo contains the source code for a Code::Blocks GDB/MI debugger plugin.
The source is being worked on in order to ensure that it has at least the same features as the current C::B 20.03 GDB plugin.

The debugger is able to debug itself and is a viable replacement for the existing GDB plugin.

## OUTSTANDING ITEMS

### High Priority
1. Build plugin using the SVN trunk source. Needs ticket 1250 merged.
2. Test plugin against SVN trunk C::B build. see 1) above
3. Persist debug data between sessions:
    Outstanding
        - watch data                            - save/load working
        - watch data edited                     - not done
        - memory dump (single memory dialog)    - not done
        - memory view (multiple memory dialog)  - not done
        - data break point                      - not done

    Save/load working
        - line break points                 - save/load working
        - count line break point            - save/load working
        - conditional line break point      - save/load working

### Medium Priority

1. Expand function argument and local variables in the watch window.
2. Add ability delete data watch point https://ftp.gnu.org/old-gnu/Manuals/gdb/html_node/gdb_31.html
3. Remote debugging 
    - use Project->Properties->"Debugger GDB/MI" tab data.

4. Re-test all items!!!! 
5. Create patch for C::B source and submit


### Low Priority

1. Create Linux project file
2. Create MacOS project file
3. Update Linux makefile build process
4. Update MSYS2 makefile build process
5. Update MacOS makefile build process
6. CPU registry dialog modify to fix value column to say 50 characters.
7. More (easy) integration of pretty printing
9. Add function break point support. See "#warning" messages.
    src\plugins\contrib-wip\debuggerGDB_MI\src\actions.cpp
        91    #warning dbg_mi::GDBBreakpoint::BreakpointType::bptFunction not supported yet!! 
        233   #warning dbg_mi::GDBBreakpoint::BreakpointType::bptFunction not supported yet!!                            
        239   #warning dbg_mi::GDBBreakpoint::BreakpointType::bptFunction not supported yet!!                            
        393   #warning dbg_mi::GDBBreakpoint::BreakpointType::bptFunction not supported yet!!                            
        399   #warning dbg_mi::GDBBreakpoint::BreakpointType::bptFunction not supported yet!!                            

    \src\plugins\contrib-wip\debuggerGDB_MI\src\plugin.cpp
        1566  #warning dbg_mi::GDBBreakpoint::BreakpointType::bptFunction not supported yet!!                             


### Future Work

1. Checkpoints (only available on linux )
2. Display the return value of a function after "step out"
3. Skipping functions - see https://sourceware.org/gdb/onlinedocs/gdb/Skipping-Over-Functions-and-Files.html
4. Record replay (only linux)

## Testing/Coding/Feature Check List

|                   Item                                      |   Date    |   Result   |
|-------------------------------------------------------------|-----------|------------|
| **Stepping**                                                |           |            |
|   * Start/Continue       (F8)                               | 26MAR2022 |    Pass    |
|   * Break debugger                                          | 26MAR2022 |    Pass    |
|   * Stop debugger        (Shift-F8)                         | 26MAR2022 |    Pass    |
|   * Run to cursor        (F4)                               | 26MAR2022 |    Pass    |
|   * Next line            (F7)                               | 26MAR2022 |    Pass    |
|   * Step Into            (Shift-F7)                         | 26MAR2022 |    Pass    |
|   * Step out             (Ctrl-F7)                          | 26MAR2022 |    Pass    |
|      * Display the return value of a function               |  Future   |   Future   |
|   * Next instruction     (Alt-F7)                           |  To test  |   To test  |
|   * Step into instruction(ALT-Shift-F7)                     |  To test  |   To test  |
|   * Set next statement                                      |  To test  |   To test  |
|   * Notification that the debugging has ended               | 26MAR2022 |    Pass    |
|   * Skipping over functions                                 |  Future   |   Future   |
|   * Skipping over files                                     |  Future   |   Future   |
|                                                             |           |            |
| **Watches**                                                 |           |            |
|   * watches dialog shows function args and local vars       | 06APR2022 |  *Partial* |
|   * Add watch before starting the debugger                  | 02APR2022 |   * NEW *  |
|   * Add watch after starting the debugger                   | 02APR2022 |    Pass    |
|   * Simple data types                                       | 26MAR2022 |    Pass    |
|   * Simple structure                                        | 26MAR2022 |    Pass    |
|   * Array of simple structures                              | 26MAR2022 |    Pass    |
|   * Complex structures                                      | 26MAR2022 |    Pass    |
|   * Edit watches                                            | 30MAR2022 |    Pass    |
|   * Watches data saved on project close                     | 02APR2022 |  * NEW *   |
|   * Watches removed after closing the project               | 02APR2022 |    Pass    |
|   * Watches removed after changing debugger                 | 02APR2022 |  *Broken*  |
|   * Watches created on project open                         | 02APR2022 |    WIP     |
|                                                             |           |            |
| **Breakpoints**                                             |           |            |
|   * Add line break point before starting the debugger       | 02APR2022 |    Pass    |
|   * Add line break point after the starting the debugger    | 26MAR2022 |    Pass    |
|   * Debug menu option to Toggle line break point (F5)       | 26MAR2022 |    Pass    |
|   * Debug menu option to Remove all breakpoints             | 26MAR2022 |    Pass    |
|   * Disable/Enable line break point via pop up menu         | 28MAR2022 |    Pass    |
|   * Remove line break point                                 | 26MAR2022 |    Pass    |
|   * Edit line break point                                   | 26MAR2022 |    Pass    |
|     * ignore count before break                             | 26MAR2022 |    Pass    |
|     * break when expression is true                         | 26MAR2022 |    Pass    |
|   * Break points still there after GDB exit                 | 03APR2022 |    Pass    |
|   * Break points data saved on project close                | 05APR2022 |    Pass    |
|   * Break points removed after closing the project          | 02APR2022 |    Pass    |
|   * Break points removed after changing debugger            | 02APR2022 |  *Broken*  |
|   * Break points created on project open                    | 05APR2022 |    Pass    |
|                                                             |           |            |
| **Data Breakpoints**                                        |           |            |
|   * Display data breakpoints in break point dialog          | 07APR2022 |    Pass    |
|   * Add data break point (right click pop up menu on var)   | 03APR2022 |  *Partial* |
|   * Add support for deleting already added data break point |  Future   |   Future   |
|   * Add remove data breakpoints to Remove all breakpoints   |  Future   |   Future   |
|   * Add support for multiple data break points              |  Future   |   Future   |
|   * Data break points data is NOT saved on project close    |  To test  |   To test  |
|                                                             |           |            |
| **Debug show Running Threads**                              |           |            |
|   * Show running threads dialog                             | 26MAR2022 |    Pass    |
|   * Close running threads dialog                            | 26MAR2022 |    Pass    |
|   * Updates on step/run                                     | 27MAR2022 |    Pass    |
|   * Thread dialog cleared on app exit                       |  To test  |   To test  |
|                                                             |           |            |
| **Debug show CPU Registers**                                |           |            |
|   * Show CPU register dialog                                | 27MAR2022 |    Pass    |
|   * Close CPU register dialog                               | 27MAR2022 |    Pass    |
|   * Update on step                                          | 27MAR2022 |    Pass    |
|   * CPU register dialog cleared on GDB exit                 | 03APR2022 |    Pass    |
|                                                             |           |            |
| **Debug show Call Stack**                                   |           |            |
|   * Show call stack dialog                                  | 26MAR2022 |    Pass    |
|   * Close call stack dialog                                 | 26MAR2022 |    Pass    |
|   * Double click on entry should open and go to the line    | 28MAR2022 |    Pass    |
|   * Call stack dialog cleared on last project close         |  To test  |   To test  |
|                                                             |           |            |
| **Debug show Disassembly**                                  |           |            |
|   * Show disassembly dialog                                 | 31MAR2022 |    Pass    |
|   * Close disassembly dialog                                | 31MAR2022 |    Pass    |
|   * Assembly only                                           | 31MAR2022 |    Pass    |
|   * Mixed mode                                              | 31MAR2022 |    Pass    |
|   * Ability to swap between assembly and mixed modes        | 31MAR2022 |    Pass    |
|   * Adjust button feature                                   | 31MAR2022 |  *Broken*  |
|   * Save disassembly to a file                              | 31MAR2022 |    Pass    |
|   * Disassembly dialog cleared on GDB exit                  | 03APR2022 |    Pass    |
|                                                             |           |            |
| **Debug -> Memory Dump Dialog**                             |           |            |
|   * Show memory dump dialog                                 | 31MAR2022 |    Pass    |
|   * Close memory dump dialog                                | 31MAR2022 |    Pass    |
|   * Show variable memory from watch dialog                  | 02APR2022 |    Pass    |
|   * Memory dump dialog not cleared on GDB exit              | 03APR2022 |    Pass    |
|   * Memory dump dialog cleared on last project close        | 03APR2022 |    Pass    |
|   * Memory dump watches data saved on project close         | 02APR2022 |    WIP     |
|   * Memory dump watches removed after changing debugger     | 02APR2022 |  * NEW *   |
|   * Memory dump watches created on project open             | 02APR2022 |  * NEW *   |
|                                                             |           |            |
| **Debug -> Memory view Dialog**                             |           |            |
|   * Show memory view dialog                                 | 26MAR2022 |    Pass    |
|   * Close memory view dialog                                | 26MAR2022 |    Pass    |
|   * Show memory information                                 | 02APR2022 |    Pass    |
|   * Show variable memory from watch dialog                  | 05APR2022 |  * NEW *   |
|   * Memory view dialog cleared on last project close        |  To test  |   To test  |
|   * Memory view watches data saved on project close         | 02APR2022 |  * NEW *   |
|   * Memory view watches removed after closing the project   | 02APR2022 |  * NEW *   |
|   * Memory view watches removed after changing debugger     | 02APR2022 |  * NEW *   |
|   * Memory view watches created on project open             | 02APR2022 |  * NEW *   |
|                                                             |           |            |
| **Show tty for console projects**                           |           |            |
|   * Show console on console app                             | 28MAR2022 |    Pass    |
|   * Do NOT show console on GUI app                          | 28MAR2022 |    Pass    |
|                                                             |           |            |
| **Projects - debugger options dialog**                      |           |            |
|   * Show debuggeroptionsprjdlg dialog                       | 30MAR2022 |    Pass    |
|   * Close debuggeroptionsprjdlg dialog                      | 30MAR2022 |    Pass    |
|   * Data saved /loaded                                      |  To test  |   To test  |
|   * Data used by GDB/MI                                     |  To test  |   To test  |
|                                                             |           |            |
| **GDB/MI Debugger configuration dialog**                    |           |            |
|   * Show debugger options dialog                            | 26MAR2022 |    Pass    |
|   * Close debugger options dialog                           | 26MAR2022 |    Pass    |
|   * Executable path save/loaded/used                        | 26MAR2022 |    Pass    |
|   * Arguments save/loaded/used                              |  To test  |   To test  |
|   * Arguments used in starting debugger                     |  To test  |   To test  |
|   * Debugger init commands save/loaded/used                 |  To test  |   To test  |
|   * Disable startup scripts checkbox save/loaded/used       |  To test  |   To test  |
|   * Watch function arguments checkbox save/loaded/used      |  To test  |   To test  |
|   * Watch local variables checkbox save/loaded/used         |  To test  |   To test  |
|   * Catch C++ exceptions checkbox save/loaded/used          |  To test  |   To test  |
|   * Evaluate expression under cursor cbox save/loaded/used  |  To test  |   To test  |
|   * Add other projects paths... checkbox save/loaded/used   |  To test  |   To test  |
|   * Do not run the debugee checkbox save/loaded/used        |  To test  |   To test  |
|   * Use python pretty printer checkbox save/loaded/used     |  To test  |   To test  |
|   * Disassembly flavor drop down list save/loaded/used      |  To test  |   To test  |
|   * Persists checkbox save/load breakpoint/watch etc        | 05APR2022 |    Pass    |
|                                                             |           |            |
| **Checkpoints on Linux**                                    |           |            |
|   * Add checkpoint                                          |  Future   |   Future   |
|   * Delete checkpoint                                       |  Future   |   Future   |
|   * Project checkpoint data saved on project close          |  Future   |   Future   |
|   * Project checkpoint removed after closing the project    |  Future   |   Future   |
|   * Project checkpoint removed after changing debugger      |  Future   |   Future   |
|                                                             |           |            |
| **Pretty Printing**                                         |           |            |
|   * Ensure pretty printing works                            |  To test  |   To test  |
|                                                             |           |            |
| **Remote Debugging**                                        |           |            |
|   * Use data from debuggeroptionsprjdlg dialog              | 02APR2022 |  *Broken*  |
|   * Ensure serial (RS232) debugging works                   | 02APR2022 |  *Broken*  |
|   * Ensure TCP debugging works                              | 02APR2022 |  *Broken*  |
|                                                             |           |            |
| **Operaiting System**                                       |           |            |
|   * Works on Windows                                        | 02APR2022 |    Pass    |
|   * Works on Linux                                          |  Future   |   Future   |
|   * Works on MacOS                                          |  Future   |   Future   |
|   * Builds on Windows via workspace                         | 02APR2022 |    Pass    |
|   * Builds on Windows via MSYS2 makefile                    |  Future   |   Future   |
|   * Builds on Linux via workspace                           |  Future   |   Future   |
|   * Builds on Linux via makefile                            |  Future   |   Future   |
|   * Builds on MacOS via workspace                           |  Future   |   Future   |
|   * Builds on MacOS via makefile                            |  Future   |   Future   |
|   * Create and test GDM/MI cbplugin for Windows             | 02APR2022 |    Pass    |
|   * Create and test GDM/MI cbplugin for Linux               |  Future   |   Future   |
|   * Create and test GDM/MI cbplugin for MacOS               |  Future   |   Future   |
|                                                             |           |            |

NOTES:
 "PASS"     - initial testing showed the item worked as expected, but changes since may have broken it... Please raise an issue if you find it broken.
 "WIP"      - working on this item
 "*Broken*" - during testing the feature/item did not work or work as expected
 "* NEW *"  - new feature that is not (or does not work) in the existing GDB debugger
 "To test"  - not tested or not tested as it relies on other items and as such it is not worth testing now.
 "Future"   - new functionality that another developer will need help with or do if time does not permit

## COMPLETED ITEMS

* 08APR2022 Done - Create patch for SVN code for C::B code changes. See https://sourceforge.net/p/codeblocks/tickets/1250/
* 07APR2022 Done - Added search paths from Project->Properties->"Debugger GDB/MI" tab to GDB startup
* 07APR2022 Done - Wired up the display of data break point to the break point debugging window
* 06APR2022 Done - Wired up the ability to create a data break point 
* 05APR2022 Done - Added function argument and local function variables to watch window (if enabled in debugger config dialog)
* 05APR2022 Done - Updated amount of logging when debugger HasDebugLog() enabled or disabled
* 05APR2022 Done - simple, count and conditional breakpoints persistencey working
* 05APR2022 Done - Added new Checkbox for save/load breakpoint/watch in teh GDB/MI config dialog
* 05Apr2022 Fixed - Debug -> Memory view dialog now displaying memory.... But C::B source needs changes!!!!
* 03APR2022 Done - Disassembly dialog cleared on GDB exit
* 03APR2022 Done - Disassembly dialog cleared on GDB exit
* 03APR2022 Done - CPU register dialog cleared on GDB exit 
* 03APR2022 Done - Memory dump dialog cleared on last project close
* 03APR2022 Updated - watch -> view memory code changes to show error on memory view and now uses GDB "-data-read-memory-bytes" command
* 02APR2022 Done - simple watch and simple line break points are persistent
* 30MAR2022 Done - Disassembly view working like the existing code (adjust is broken)
* 30MAR2022 Done - Wired up Project->Properties->"Debugger GDB/MI" tab. Loads and saves data only.
* 30MAR2022 Done - Conditional break points
* 30MAR2022 Done - Edit watches now working
* 28MAR2022 Done - Examine Memory Dialog now working
* 28MAR2022 Done - fixed Disable/enable break point via pop up menu
* 28MAR2022 Done - Show tty for console projects 
* 27MAR2022 Done - CPU Registers showing and updating
* 26MAR2022 Fixed - display of structures with multiple depths now working
* 26MAR2022 Done - Debug "simple" source code sample with the following "data" types:
* 26MAR2022 Done - First pass of the check list completed
* 26MAR2022 Done - Project file now creates the debugger_gdbmi.cbplugin file with the non striped DLL (still includes debugging)
* 25MAR2022 Done - Update logged data to be more readable/understanding
* 25MAR2022 Fixed - removed array watch limit of 100
* 25MAR2022 Fixed - updates of array items > 9
* 24MAR2022 Done - Update logging - merge new logging code
* 24MAR2022 Done - Fix warnings
* 24MAR2022 Done - Update file header - C::B GPL text
* 24MAR2022 Done - Add more logging to help find issues

CODING notes:
GDB Manual that seems to have more examples than the latest:  https://www.manpagez.com/info/gdb/gdb_252.php


========
** C::B SVN SRC build changes:**
1) Add SRC\BASE\TINYXML2 directory.