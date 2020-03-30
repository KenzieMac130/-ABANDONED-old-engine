//This file is automatically generated by generator.lua from https://github.com/cimgui/cimgui
//based on imgui.h file version "1.76 WIP" from Dear ImGui https://github.com/ocornut/imgui
#ifdef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
typedef struct ImGuiPtrOrIndex ImGuiPtrOrIndex;
typedef struct ImGuiShrinkWidthItem ImGuiShrinkWidthItem;
typedef struct ImVec2ih ImVec2ih;
typedef struct ImVec1 ImVec1;
typedef struct ImGuiWindowSettings ImGuiWindowSettings;
typedef struct ImGuiWindowTempData ImGuiWindowTempData;
typedef struct ImGuiWindow ImGuiWindow;
typedef struct ImGuiTabItem ImGuiTabItem;
typedef struct ImGuiTabBar ImGuiTabBar;
typedef struct ImGuiStyleMod ImGuiStyleMod;
typedef struct ImGuiSettingsHandler ImGuiSettingsHandler;
typedef struct ImGuiPopupData ImGuiPopupData;
typedef struct ImGuiNextItemData ImGuiNextItemData;
typedef struct ImGuiNextWindowData ImGuiNextWindowData;
typedef struct ImGuiNavMoveResult ImGuiNavMoveResult;
typedef struct ImGuiMenuColumns ImGuiMenuColumns;
typedef struct ImGuiItemHoveredDataBackup ImGuiItemHoveredDataBackup;
typedef struct ImGuiInputTextState ImGuiInputTextState;
typedef struct ImGuiGroupData ImGuiGroupData;
typedef struct ImGuiDataTypeInfo ImGuiDataTypeInfo;
typedef struct ImGuiContext ImGuiContext;
typedef struct ImGuiColumns ImGuiColumns;
typedef struct ImGuiColumnData ImGuiColumnData;
typedef struct ImGuiColorMod ImGuiColorMod;
typedef struct ImDrawListSharedData ImDrawListSharedData;
typedef struct ImDrawDataBuilder ImDrawDataBuilder;
typedef struct ImRect ImRect;
typedef struct ImBitVector ImBitVector;

struct ImBitVector;
struct ImRect;
struct ImDrawDataBuilder;
struct ImDrawListSharedData;
struct ImGuiColorMod;
struct ImGuiColumnData;
struct ImGuiColumns;
struct ImGuiContext;
struct ImGuiDataTypeInfo;
struct ImGuiGroupData;
struct ImGuiInputTextState;
struct ImGuiItemHoveredDataBackup;
struct ImGuiMenuColumns;
struct ImGuiNavMoveResult;
struct ImGuiNextWindowData;
struct ImGuiNextItemData;
struct ImGuiPopupData;
struct ImGuiSettingsHandler;
struct ImGuiStyleMod;
struct ImGuiTabBar;
struct ImGuiTabItem;
struct ImGuiWindow;
struct ImGuiWindowTempData;
struct ImGuiWindowSettings;
typedef int ImGuiLayoutType;
typedef int ImGuiButtonFlags;
typedef int ImGuiColumnsFlags;
typedef int ImGuiDragFlags;
typedef int ImGuiItemFlags;
typedef int ImGuiItemStatusFlags;
typedef int ImGuiNavHighlightFlags;
typedef int ImGuiNavDirSourceFlags;
typedef int ImGuiNavMoveFlags;
typedef int ImGuiNextItemDataFlags;
typedef int ImGuiNextWindowDataFlags;
typedef int ImGuiSeparatorFlags;
typedef int ImGuiSliderFlags;
typedef int ImGuiTextFlags;
typedef int ImGuiTooltipFlags;
extern ImGuiContext* GImGui;
typedef FILE* ImFileHandle;
typedef int ImPoolIdx;typedef struct ImVector_unsigned_char {int Size;int Capacity;unsigned char* Data;} ImVector_unsigned_char;
typedef struct ImVector_ImGuiSettingsHandler {int Size;int Capacity;ImGuiSettingsHandler* Data;} ImVector_ImGuiSettingsHandler;
typedef struct ImVector_ImGuiStyleMod {int Size;int Capacity;ImGuiStyleMod* Data;} ImVector_ImGuiStyleMod;
typedef struct ImVector_ImGuiPopupData {int Size;int Capacity;ImGuiPopupData* Data;} ImVector_ImGuiPopupData;
typedef struct ImVector_ImGuiID {int Size;int Capacity;ImGuiID* Data;} ImVector_ImGuiID;
typedef struct ImVector_ImGuiWindowPtr {int Size;int Capacity;ImGuiWindow** Data;} ImVector_ImGuiWindowPtr;
typedef struct ImVector_ImGuiShrinkWidthItem {int Size;int Capacity;ImGuiShrinkWidthItem* Data;} ImVector_ImGuiShrinkWidthItem;
typedef struct ImVector_ImGuiTabItem {int Size;int Capacity;ImGuiTabItem* Data;} ImVector_ImGuiTabItem;
typedef struct ImVector_ImGuiColumns {int Size;int Capacity;ImGuiColumns* Data;} ImVector_ImGuiColumns;
typedef struct ImVector_ImGuiGroupData {int Size;int Capacity;ImGuiGroupData* Data;} ImVector_ImGuiGroupData;
typedef struct ImVector_ImGuiColumnData {int Size;int Capacity;ImGuiColumnData* Data;} ImVector_ImGuiColumnData;
typedef struct ImVector_ImGuiItemFlags {int Size;int Capacity;ImGuiItemFlags* Data;} ImVector_ImGuiItemFlags;
typedef struct ImVector_ImGuiPtrOrIndex {int Size;int Capacity;ImGuiPtrOrIndex* Data;} ImVector_ImGuiPtrOrIndex;
typedef struct ImVector_ImDrawListPtr {int Size;int Capacity;ImDrawList** Data;} ImVector_ImDrawListPtr;
typedef struct ImVector_ImGuiColorMod {int Size;int Capacity;ImGuiColorMod* Data;} ImVector_ImGuiColorMod;
typedef struct ImVector_ImGuiWindowSettings {int Size;int Capacity;ImGuiWindowSettings* Data;} ImVector_ImGuiWindowSettings;
typedef struct ImChunkStream_ImGuiWindowSettings {ImVector_ImGuiWindowSettings Buf;} ImChunkStream_ImGuiWindowSettings;
typedef struct ImVector_ImGuiTabBar {int Size;int Capacity;ImGuiTabBar* Data;} ImVector_ImGuiTabBar;
typedef struct ImPool_ImGuiTabBar {ImVector_ImGuiTabBar Buf;ImGuiStorage Map;ImPoolIdx FreeIdx;} ImPool_ImGuiTabBar;

typedef struct
{
   int where;
   int insert_length;
   int delete_length;
   int char_storage;
} StbUndoRecord;
typedef struct
{
   StbUndoRecord undo_rec [99];
   ImWchar undo_char[999];
   short undo_point, redo_point;
   int undo_char_point, redo_char_point;
} StbUndoState;
typedef struct
{
   int cursor;
   int select_start;
   int select_end;
   unsigned char insert_mode;
   unsigned char cursor_at_end_of_line;
   unsigned char initialized;
   unsigned char has_preferred_x;
   unsigned char single_line;
   unsigned char padding1, padding2, padding3;
   float preferred_x;
   StbUndoState undostate;
} STB_TexteditState;
typedef struct
{
   float x0,x1;
   float baseline_y_delta;
   float ymin,ymax;
   int num_chars;
} StbTexteditRow;
struct ImBitVector
{
    ImVector_ImU32 Storage;
};
typedef enum {
    ImGuiButtonFlags_None = 0,
    ImGuiButtonFlags_Repeat = 1 << 0,
    ImGuiButtonFlags_PressedOnClick = 1 << 1,
    ImGuiButtonFlags_PressedOnClickRelease = 1 << 2,
    ImGuiButtonFlags_PressedOnClickReleaseAnywhere = 1 << 3,
    ImGuiButtonFlags_PressedOnRelease = 1 << 4,
    ImGuiButtonFlags_PressedOnDoubleClick = 1 << 5,
    ImGuiButtonFlags_PressedOnDragDropHold = 1 << 6,
    ImGuiButtonFlags_FlattenChildren = 1 << 7,
    ImGuiButtonFlags_AllowItemOverlap = 1 << 8,
    ImGuiButtonFlags_DontClosePopups = 1 << 9,
    ImGuiButtonFlags_Disabled = 1 << 10,
    ImGuiButtonFlags_AlignTextBaseLine = 1 << 11,
    ImGuiButtonFlags_NoKeyModifiers = 1 << 12,
    ImGuiButtonFlags_NoHoldingActiveId = 1 << 13,
    ImGuiButtonFlags_NoNavFocus = 1 << 14,
    ImGuiButtonFlags_NoHoveredOnFocus = 1 << 15,
    ImGuiButtonFlags_MouseButtonLeft = 1 << 16,
    ImGuiButtonFlags_MouseButtonRight = 1 << 17,
    ImGuiButtonFlags_MouseButtonMiddle = 1 << 18,
    ImGuiButtonFlags_MouseButtonMask_ = ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle,
    ImGuiButtonFlags_MouseButtonShift_ = 16,
    ImGuiButtonFlags_MouseButtonDefault_ = ImGuiButtonFlags_MouseButtonLeft,
    ImGuiButtonFlags_PressedOnMask_ = ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnClickReleaseAnywhere | ImGuiButtonFlags_PressedOnRelease | ImGuiButtonFlags_PressedOnDoubleClick | ImGuiButtonFlags_PressedOnDragDropHold,
    ImGuiButtonFlags_PressedOnDefault_ = ImGuiButtonFlags_PressedOnClickRelease
}ImGuiButtonFlags_;
typedef enum {
    ImGuiSliderFlags_None = 0,
    ImGuiSliderFlags_Vertical = 1 << 0
}ImGuiSliderFlags_;
typedef enum {
    ImGuiDragFlags_None = 0,
    ImGuiDragFlags_Vertical = 1 << 0
}ImGuiDragFlags_;
typedef enum {
    ImGuiColumnsFlags_None = 0,
    ImGuiColumnsFlags_NoBorder = 1 << 0,
    ImGuiColumnsFlags_NoResize = 1 << 1,
    ImGuiColumnsFlags_NoPreserveWidths = 1 << 2,
    ImGuiColumnsFlags_NoForceWithinWindow = 1 << 3,
    ImGuiColumnsFlags_GrowParentContentsSize= 1 << 4
}ImGuiColumnsFlags_;
typedef enum {
    ImGuiSelectableFlags_NoHoldingActiveID = 1 << 20,
    ImGuiSelectableFlags_SelectOnClick = 1 << 21,
    ImGuiSelectableFlags_SelectOnRelease = 1 << 22,
    ImGuiSelectableFlags_DrawFillAvailWidth = 1 << 23,
    ImGuiSelectableFlags_DrawHoveredWhenHeld= 1 << 24,
    ImGuiSelectableFlags_SetNavIdOnHover = 1 << 25
}ImGuiSelectableFlagsPrivate_;
typedef enum {
    ImGuiTreeNodeFlags_ClipLabelForTrailingButton = 1 << 20
}ImGuiTreeNodeFlagsPrivate_;
typedef enum {
    ImGuiSeparatorFlags_None = 0,
    ImGuiSeparatorFlags_Horizontal = 1 << 0,
    ImGuiSeparatorFlags_Vertical = 1 << 1,
    ImGuiSeparatorFlags_SpanAllColumns = 1 << 2
}ImGuiSeparatorFlags_;
typedef enum {
    ImGuiItemFlags_None = 0,
    ImGuiItemFlags_NoTabStop = 1 << 0,
    ImGuiItemFlags_ButtonRepeat = 1 << 1,
    ImGuiItemFlags_Disabled = 1 << 2,
    ImGuiItemFlags_NoNav = 1 << 3,
    ImGuiItemFlags_NoNavDefaultFocus = 1 << 4,
    ImGuiItemFlags_SelectableDontClosePopup = 1 << 5,
    ImGuiItemFlags_MixedValue = 1 << 6,
    ImGuiItemFlags_Default_ = 0
}ImGuiItemFlags_;
typedef enum {
    ImGuiItemStatusFlags_None = 0,
    ImGuiItemStatusFlags_HoveredRect = 1 << 0,
    ImGuiItemStatusFlags_HasDisplayRect = 1 << 1,
    ImGuiItemStatusFlags_Edited = 1 << 2,
    ImGuiItemStatusFlags_ToggledSelection = 1 << 3,
    ImGuiItemStatusFlags_ToggledOpen = 1 << 4,
    ImGuiItemStatusFlags_HasDeactivated = 1 << 5,
    ImGuiItemStatusFlags_Deactivated = 1 << 6
}ImGuiItemStatusFlags_;
typedef enum {
    ImGuiTextFlags_None = 0,
    ImGuiTextFlags_NoWidthForLargeClippedText = 1 << 0
}ImGuiTextFlags_;
typedef enum {
    ImGuiTooltipFlags_None = 0,
    ImGuiTooltipFlags_OverridePreviousTooltip = 1 << 0
}ImGuiTooltipFlags_;
typedef enum {
    ImGuiLayoutType_Horizontal = 0,
    ImGuiLayoutType_Vertical = 1
}ImGuiLayoutType_;
typedef enum {
    ImGuiLogType_None = 0,
    ImGuiLogType_TTY,
    ImGuiLogType_File,
    ImGuiLogType_Buffer,
    ImGuiLogType_Clipboard
}ImGuiLogType;
typedef enum {
    ImGuiAxis_None = -1,
    ImGuiAxis_X = 0,
    ImGuiAxis_Y = 1
}ImGuiAxis;
typedef enum {
    ImGuiPlotType_Lines,
    ImGuiPlotType_Histogram
}ImGuiPlotType;
typedef enum {
    ImGuiInputSource_None = 0,
    ImGuiInputSource_Mouse,
    ImGuiInputSource_Nav,
    ImGuiInputSource_NavKeyboard,
    ImGuiInputSource_NavGamepad,
    ImGuiInputSource_COUNT
}ImGuiInputSource;
typedef enum {
    ImGuiInputReadMode_Down,
    ImGuiInputReadMode_Pressed,
    ImGuiInputReadMode_Released,
    ImGuiInputReadMode_Repeat,
    ImGuiInputReadMode_RepeatSlow,
    ImGuiInputReadMode_RepeatFast
}ImGuiInputReadMode;
typedef enum {
    ImGuiNavHighlightFlags_None = 0,
    ImGuiNavHighlightFlags_TypeDefault = 1 << 0,
    ImGuiNavHighlightFlags_TypeThin = 1 << 1,
    ImGuiNavHighlightFlags_AlwaysDraw = 1 << 2,
    ImGuiNavHighlightFlags_NoRounding = 1 << 3
}ImGuiNavHighlightFlags_;
typedef enum {
    ImGuiNavDirSourceFlags_None = 0,
    ImGuiNavDirSourceFlags_Keyboard = 1 << 0,
    ImGuiNavDirSourceFlags_PadDPad = 1 << 1,
    ImGuiNavDirSourceFlags_PadLStick = 1 << 2
}ImGuiNavDirSourceFlags_;
typedef enum {
    ImGuiNavMoveFlags_None = 0,
    ImGuiNavMoveFlags_LoopX = 1 << 0,
    ImGuiNavMoveFlags_LoopY = 1 << 1,
    ImGuiNavMoveFlags_WrapX = 1 << 2,
    ImGuiNavMoveFlags_WrapY = 1 << 3,
    ImGuiNavMoveFlags_AllowCurrentNavId = 1 << 4,
    ImGuiNavMoveFlags_AlsoScoreVisibleSet = 1 << 5,
    ImGuiNavMoveFlags_ScrollToEdge = 1 << 6
}ImGuiNavMoveFlags_;
typedef enum {
    ImGuiNavForward_None,
    ImGuiNavForward_ForwardQueued,
    ImGuiNavForward_ForwardActive
}ImGuiNavForward;
typedef enum {
    ImGuiNavLayer_Main = 0,
    ImGuiNavLayer_Menu = 1,
    ImGuiNavLayer_COUNT
}ImGuiNavLayer;
typedef enum {
    ImGuiPopupPositionPolicy_Default,
    ImGuiPopupPositionPolicy_ComboBox
}ImGuiPopupPositionPolicy;
struct ImVec1
{
    float x;
};
struct ImVec2ih
{
    short x, y;
};
struct ImRect
{
    ImVec2 Min;
    ImVec2 Max;
};
struct ImGuiDataTypeInfo
{
    size_t Size;
    const char* PrintFmt;
    const char* ScanFmt;
};
struct ImGuiColorMod
{
    ImGuiCol Col;
    ImVec4 BackupValue;
};
struct ImGuiStyleMod
{
    ImGuiStyleVar VarIdx;
    union { int BackupInt[2]; float BackupFloat[2]; };
};
struct ImGuiGroupData
{
    ImVec2 BackupCursorPos;
    ImVec2 BackupCursorMaxPos;
    ImVec1 BackupIndent;
    ImVec1 BackupGroupOffset;
    ImVec2 BackupCurrLineSize;
    float BackupCurrLineTextBaseOffset;
    ImGuiID BackupActiveIdIsAlive;
    bool BackupActiveIdPreviousFrameIsAlive;
    bool EmitItem;
};
struct ImGuiMenuColumns
{
    float Spacing;
    float Width, NextWidth;
    float Pos[3], NextWidths[3];
};
struct ImGuiInputTextState
{
    ImGuiID ID;
    int CurLenW, CurLenA;
    ImVector_ImWchar TextW;
    ImVector_char TextA;
    ImVector_char InitialTextA;
    bool TextAIsValid;
    int BufCapacityA;
    float ScrollX;
    STB_TexteditState Stb;
    float CursorAnim;
    bool CursorFollow;
    bool SelectedAllMouseLock;
    ImGuiInputTextFlags UserFlags;
    ImGuiInputTextCallback UserCallback;
    void* UserCallbackData;
};
struct ImGuiWindowSettings
{
    ImGuiID ID;
    ImVec2ih Pos;
    ImVec2ih Size;
    bool Collapsed;
};
struct ImGuiSettingsHandler
{
    const char* TypeName;
    ImGuiID TypeHash;
    void* (*ReadOpenFn)(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name);
    void (*ReadLineFn)(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line);
    void (*WriteAllFn)(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out_buf);
    void* UserData;
};
struct ImGuiPopupData
{
    ImGuiID PopupId;
    ImGuiWindow* Window;
    ImGuiWindow* SourceWindow;
    int OpenFrameCount;
    ImGuiID OpenParentId;
    ImVec2 OpenPopupPos;
    ImVec2 OpenMousePos;
};
struct ImGuiColumnData
{
    float OffsetNorm;
    float OffsetNormBeforeResize;
    ImGuiColumnsFlags Flags;
    ImRect ClipRect;
};
struct ImGuiColumns
{
    ImGuiID ID;
    ImGuiColumnsFlags Flags;
    bool IsFirstFrame;
    bool IsBeingResized;
    int Current;
    int Count;
    float OffMinX, OffMaxX;
    float LineMinY, LineMaxY;
    float HostCursorPosY;
    float HostCursorMaxPosX;
    ImRect HostClipRect;
    ImRect HostWorkRect;
    ImVector_ImGuiColumnData Columns;
    ImDrawListSplitter Splitter;
};
struct ImDrawListSharedData
{
    ImVec2 TexUvWhitePixel;
    ImFont* Font;
    float FontSize;
    float CurveTessellationTol;
    float CircleSegmentMaxError;
    ImVec4 ClipRectFullscreen;
    ImDrawListFlags InitialFlags;
    ImVec2 ArcFastVtx[12 * 1];
    ImU8 CircleSegmentCounts[64];
};
struct ImDrawDataBuilder
{
    ImVector_ImDrawListPtr Layers[2];
};
struct ImGuiNavMoveResult
{
    ImGuiWindow* Window;
    ImGuiID ID;
    ImGuiID FocusScopeId;
    float DistBox;
    float DistCenter;
    float DistAxial;
    ImRect RectRel;
};
typedef enum {
    ImGuiNextWindowDataFlags_None = 0,
    ImGuiNextWindowDataFlags_HasPos = 1 << 0,
    ImGuiNextWindowDataFlags_HasSize = 1 << 1,
    ImGuiNextWindowDataFlags_HasContentSize = 1 << 2,
    ImGuiNextWindowDataFlags_HasCollapsed = 1 << 3,
    ImGuiNextWindowDataFlags_HasSizeConstraint = 1 << 4,
    ImGuiNextWindowDataFlags_HasFocus = 1 << 5,
    ImGuiNextWindowDataFlags_HasBgAlpha = 1 << 6
}ImGuiNextWindowDataFlags_;
struct ImGuiNextWindowData
{
    ImGuiNextWindowDataFlags Flags;
    ImGuiCond PosCond;
    ImGuiCond SizeCond;
    ImGuiCond CollapsedCond;
    ImVec2 PosVal;
    ImVec2 PosPivotVal;
    ImVec2 SizeVal;
    ImVec2 ContentSizeVal;
    bool CollapsedVal;
    ImRect SizeConstraintRect;
    ImGuiSizeCallback SizeCallback;
    void* SizeCallbackUserData;
    float BgAlphaVal;
    ImVec2 MenuBarOffsetMinVal;
};
typedef enum {
    ImGuiNextItemDataFlags_None = 0,
    ImGuiNextItemDataFlags_HasWidth = 1 << 0,
    ImGuiNextItemDataFlags_HasOpen = 1 << 1
}ImGuiNextItemDataFlags_;
struct ImGuiNextItemData
{
    ImGuiNextItemDataFlags Flags;
    float Width;
    ImGuiID FocusScopeId;
    ImGuiCond OpenCond;
    bool OpenVal;
};
struct ImGuiShrinkWidthItem
{
    int Index;
    float Width;
};
struct ImGuiPtrOrIndex
{
    void* Ptr;
    int Index;
};
struct ImGuiContext
{
    bool Initialized;
    bool FontAtlasOwnedByContext;
    ImGuiIO IO;
    ImGuiStyle Style;
    ImFont* Font;
    float FontSize;
    float FontBaseSize;
    ImDrawListSharedData DrawListSharedData;
    double Time;
    int FrameCount;
    int FrameCountEnded;
    int FrameCountRendered;
    bool WithinFrameScope;
    bool WithinFrameScopeWithImplicitWindow;
    bool WithinEndChild;
    ImVector_ImGuiWindowPtr Windows;
    ImVector_ImGuiWindowPtr WindowsFocusOrder;
    ImVector_ImGuiWindowPtr WindowsTempSortBuffer;
    ImVector_ImGuiWindowPtr CurrentWindowStack;
    ImGuiStorage WindowsById;
    int WindowsActiveCount;
    ImGuiWindow* CurrentWindow;
    ImGuiWindow* HoveredWindow;
    ImGuiWindow* HoveredRootWindow;
    ImGuiWindow* MovingWindow;
    ImGuiWindow* WheelingWindow;
    ImVec2 WheelingWindowRefMousePos;
    float WheelingWindowTimer;
    ImGuiID HoveredId;
    bool HoveredIdAllowOverlap;
    ImGuiID HoveredIdPreviousFrame;
    float HoveredIdTimer;
    float HoveredIdNotActiveTimer;
    ImGuiID ActiveId;
    ImGuiID ActiveIdIsAlive;
    float ActiveIdTimer;
    bool ActiveIdIsJustActivated;
    bool ActiveIdAllowOverlap;
    bool ActiveIdHasBeenPressedBefore;
    bool ActiveIdHasBeenEditedBefore;
    bool ActiveIdHasBeenEditedThisFrame;
    ImU32 ActiveIdUsingNavDirMask;
    ImU32 ActiveIdUsingNavInputMask;
    ImU64 ActiveIdUsingKeyInputMask;
    ImVec2 ActiveIdClickOffset;
    ImGuiWindow* ActiveIdWindow;
    ImGuiInputSource ActiveIdSource;
    int ActiveIdMouseButton;
    ImGuiID ActiveIdPreviousFrame;
    bool ActiveIdPreviousFrameIsAlive;
    bool ActiveIdPreviousFrameHasBeenEditedBefore;
    ImGuiWindow* ActiveIdPreviousFrameWindow;
    ImGuiID LastActiveId;
    float LastActiveIdTimer;
    ImGuiNextWindowData NextWindowData;
    ImGuiNextItemData NextItemData;
    ImVector_ImGuiColorMod ColorModifiers;
    ImVector_ImGuiStyleMod StyleModifiers;
    ImVector_ImFontPtr FontStack;
    ImVector_ImGuiPopupData OpenPopupStack;
    ImVector_ImGuiPopupData BeginPopupStack;
    ImGuiWindow* NavWindow;
    ImGuiID NavId;
    ImGuiID NavFocusScopeId;
    ImGuiID NavActivateId;
    ImGuiID NavActivateDownId;
    ImGuiID NavActivatePressedId;
    ImGuiID NavInputId;
    ImGuiID NavJustTabbedId;
    ImGuiID NavJustMovedToId;
    ImGuiID NavJustMovedToFocusScopeId;
    ImGuiID NavNextActivateId;
    ImGuiInputSource NavInputSource;
    ImRect NavScoringRectScreen;
    int NavScoringCount;
    ImGuiNavLayer NavLayer;
    int NavIdTabCounter;
    bool NavIdIsAlive;
    bool NavMousePosDirty;
    bool NavDisableHighlight;
    bool NavDisableMouseHover;
    bool NavAnyRequest;
    bool NavInitRequest;
    bool NavInitRequestFromMove;
    ImGuiID NavInitResultId;
    ImRect NavInitResultRectRel;
    bool NavMoveFromClampedRefRect;
    bool NavMoveRequest;
    ImGuiNavMoveFlags NavMoveRequestFlags;
    ImGuiNavForward NavMoveRequestForward;
    ImGuiDir NavMoveDir, NavMoveDirLast;
    ImGuiDir NavMoveClipDir;
    ImGuiNavMoveResult NavMoveResultLocal;
    ImGuiNavMoveResult NavMoveResultLocalVisibleSet;
    ImGuiNavMoveResult NavMoveResultOther;
    ImGuiWindow* NavWindowingTarget;
    ImGuiWindow* NavWindowingTargetAnim;
    ImGuiWindow* NavWindowingList;
    float NavWindowingTimer;
    float NavWindowingHighlightAlpha;
    bool NavWindowingToggleLayer;
    ImGuiWindow* FocusRequestCurrWindow;
    ImGuiWindow* FocusRequestNextWindow;
    int FocusRequestCurrCounterRegular;
    int FocusRequestCurrCounterTabStop;
    int FocusRequestNextCounterRegular;
    int FocusRequestNextCounterTabStop;
    bool FocusTabPressed;
    ImDrawData DrawData;
    ImDrawDataBuilder DrawDataBuilder;
    float DimBgRatio;
    ImDrawList BackgroundDrawList;
    ImDrawList ForegroundDrawList;
    ImGuiMouseCursor MouseCursor;
    bool DragDropActive;
    bool DragDropWithinSource;
    bool DragDropWithinTarget;
    ImGuiDragDropFlags DragDropSourceFlags;
    int DragDropSourceFrameCount;
    int DragDropMouseButton;
    ImGuiPayload DragDropPayload;
    ImRect DragDropTargetRect;
    ImGuiID DragDropTargetId;
    ImGuiDragDropFlags DragDropAcceptFlags;
    float DragDropAcceptIdCurrRectSurface;
    ImGuiID DragDropAcceptIdCurr;
    ImGuiID DragDropAcceptIdPrev;
    int DragDropAcceptFrameCount;
    ImVector_unsigned_char DragDropPayloadBufHeap;
    unsigned char DragDropPayloadBufLocal[16];
    ImGuiTabBar* CurrentTabBar;
    ImPool_ImGuiTabBar TabBars;
    ImVector_ImGuiPtrOrIndex CurrentTabBarStack;
    ImVector_ImGuiShrinkWidthItem ShrinkWidthBuffer;
    ImVec2 LastValidMousePos;
    ImGuiInputTextState InputTextState;
    ImFont InputTextPasswordFont;
    ImGuiID TempInputId;
    ImGuiColorEditFlags ColorEditOptions;
    float ColorEditLastHue;
    float ColorEditLastSat;
    float ColorEditLastColor[3];
    ImVec4 ColorPickerRef;
    bool DragCurrentAccumDirty;
    float DragCurrentAccum;
    float DragSpeedDefaultRatio;
    float ScrollbarClickDeltaToGrabCenter;
    int TooltipOverrideCount;
    ImVector_char PrivateClipboard;
    ImVector_ImGuiID MenusIdSubmittedThisFrame;
    ImVec2 PlatformImePos;
    ImVec2 PlatformImeLastPos;
    bool SettingsLoaded;
    float SettingsDirtyTimer;
    ImGuiTextBuffer SettingsIniData;
    ImVector_ImGuiSettingsHandler SettingsHandlers;
    ImChunkStream_ImGuiWindowSettings SettingsWindows;
    bool LogEnabled;
    ImGuiLogType LogType;
    ImFileHandle LogFile;
    ImGuiTextBuffer LogBuffer;
    float LogLinePosY;
    bool LogLineFirstItem;
    int LogDepthRef;
    int LogDepthToExpand;
    int LogDepthToExpandDefault;
    bool DebugItemPickerActive;
    ImGuiID DebugItemPickerBreakId;
    float FramerateSecPerFrame[120];
    int FramerateSecPerFrameIdx;
    float FramerateSecPerFrameAccum;
    int WantCaptureMouseNextFrame;
    int WantCaptureKeyboardNextFrame;
    int WantTextInputNextFrame;
    char TempBuffer[1024*3+1];
};
struct ImGuiWindowTempData
{
    ImVec2 CursorPos;
    ImVec2 CursorPosPrevLine;
    ImVec2 CursorStartPos;
    ImVec2 CursorMaxPos;
    ImVec2 CurrLineSize;
    ImVec2 PrevLineSize;
    float CurrLineTextBaseOffset;
    float PrevLineTextBaseOffset;
    ImVec1 Indent;
    ImVec1 ColumnsOffset;
    ImVec1 GroupOffset;
    ImGuiID LastItemId;
    ImGuiItemStatusFlags LastItemStatusFlags;
    ImRect LastItemRect;
    ImRect LastItemDisplayRect;
    ImGuiNavLayer NavLayerCurrent;
    int NavLayerCurrentMask;
    int NavLayerActiveMask;
    int NavLayerActiveMaskNext;
    ImGuiID NavFocusScopeIdCurrent;
    bool NavHideHighlightOneFrame;
    bool NavHasScroll;
    bool MenuBarAppending;
    ImVec2 MenuBarOffset;
    ImGuiMenuColumns MenuColumns;
    int TreeDepth;
    ImU32 TreeJumpToParentOnPopMask;
    ImVector_ImGuiWindowPtr ChildWindows;
    ImGuiStorage* StateStorage;
    ImGuiColumns* CurrentColumns;
    ImGuiLayoutType LayoutType;
    ImGuiLayoutType ParentLayoutType;
    int FocusCounterRegular;
    int FocusCounterTabStop;
    ImGuiItemFlags ItemFlags;
    float ItemWidth;
    float TextWrapPos;
    ImVector_ImGuiItemFlags ItemFlagsStack;
    ImVector_float ItemWidthStack;
    ImVector_float TextWrapPosStack;
    ImVector_ImGuiGroupData GroupStack;
    short StackSizesBackup[6];
};
struct ImGuiWindow
{
    char* Name;
    ImGuiID ID;
    ImGuiWindowFlags Flags;
    ImVec2 Pos;
    ImVec2 Size;
    ImVec2 SizeFull;
    ImVec2 ContentSize;
    ImVec2 ContentSizeExplicit;
    ImVec2 WindowPadding;
    float WindowRounding;
    float WindowBorderSize;
    int NameBufLen;
    ImGuiID MoveId;
    ImGuiID ChildId;
    ImVec2 Scroll;
    ImVec2 ScrollMax;
    ImVec2 ScrollTarget;
    ImVec2 ScrollTargetCenterRatio;
    ImVec2 ScrollbarSizes;
    bool ScrollbarX, ScrollbarY;
    bool Active;
    bool WasActive;
    bool WriteAccessed;
    bool Collapsed;
    bool WantCollapseToggle;
    bool SkipItems;
    bool Appearing;
    bool Hidden;
    bool IsFallbackWindow;
    bool HasCloseButton;
    signed char ResizeBorderHeld;
    short BeginCount;
    short BeginOrderWithinParent;
    short BeginOrderWithinContext;
    ImGuiID PopupId;
    ImS8 AutoFitFramesX, AutoFitFramesY;
    ImS8 AutoFitChildAxises;
    bool AutoFitOnlyGrows;
    ImGuiDir AutoPosLastDirection;
    int HiddenFramesCanSkipItems;
    int HiddenFramesCannotSkipItems;
    ImGuiCond SetWindowPosAllowFlags;
    ImGuiCond SetWindowSizeAllowFlags;
    ImGuiCond SetWindowCollapsedAllowFlags;
    ImVec2 SetWindowPosVal;
    ImVec2 SetWindowPosPivot;
    ImVector_ImGuiID IDStack;
    ImGuiWindowTempData DC;
    ImRect OuterRectClipped;
    ImRect InnerRect;
    ImRect InnerClipRect;
    ImRect WorkRect;
    ImRect ClipRect;
    ImRect ContentRegionRect;
    int LastFrameActive;
    float LastTimeActive;
    float ItemWidthDefault;
    ImGuiStorage StateStorage;
    ImVector_ImGuiColumns ColumnsStorage;
    float FontWindowScale;
    int SettingsOffset;
    ImDrawList* DrawList;
    ImDrawList DrawListInst;
    ImGuiWindow* ParentWindow;
    ImGuiWindow* RootWindow;
    ImGuiWindow* RootWindowForTitleBarHighlight;
    ImGuiWindow* RootWindowForNav;
    ImGuiWindow* NavLastChildNavWindow;
    ImGuiID NavLastIds[ImGuiNavLayer_COUNT];
    ImRect NavRectRel[ImGuiNavLayer_COUNT];
    bool MemoryCompacted;
    int MemoryDrawListIdxCapacity;
    int MemoryDrawListVtxCapacity;
};
struct ImGuiItemHoveredDataBackup
{
    ImGuiID LastItemId;
    ImGuiItemStatusFlags LastItemStatusFlags;
    ImRect LastItemRect;
    ImRect LastItemDisplayRect;
};
typedef enum {
    ImGuiTabBarFlags_DockNode = 1 << 20,
    ImGuiTabBarFlags_IsFocused = 1 << 21,
    ImGuiTabBarFlags_SaveSettings = 1 << 22
}ImGuiTabBarFlagsPrivate_;
typedef enum {
    ImGuiTabItemFlags_NoCloseButton = 1 << 20
}ImGuiTabItemFlagsPrivate_;
struct ImGuiTabItem
{
    ImGuiID ID;
    ImGuiTabItemFlags Flags;
    int LastFrameVisible;
    int LastFrameSelected;
    int NameOffset;
    float Offset;
    float Width;
    float ContentWidth;
};
struct ImGuiTabBar
{
    ImVector_ImGuiTabItem Tabs;
    ImGuiID ID;
    ImGuiID SelectedTabId;
    ImGuiID NextSelectedTabId;
    ImGuiID VisibleTabId;
    int CurrFrameVisible;
    int PrevFrameVisible;
    ImRect BarRect;
    float LastTabContentHeight;
    float OffsetMax;
    float OffsetMaxIdeal;
    float OffsetNextTab;
    float ScrollingAnim;
    float ScrollingTarget;
    float ScrollingTargetDistToVisibility;
    float ScrollingSpeed;
    ImGuiTabBarFlags Flags;
    ImGuiID ReorderRequestTabId;
    ImS8 ReorderRequestDir;
    bool WantLayout;
    bool VisibleTabWasSubmitted;
    short LastTabItemIdx;
    ImVec2 FramePadding;
    ImGuiTextBuffer TabsNames;
};
#endif
