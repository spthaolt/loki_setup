#ifndef __carbonres_h__
#define __carbonres_h__

#include <carbon/carbon.h>

// Constants used to identify Carbon resources
#define OPTION_GROUP_ID         128
#define CLASS_GROUP_ID          129
#define COPY_GROUP_ID           130
#define DONE_GROUP_ID           131
#define ABORT_GROUP_ID          132
#define WARNING_GROUP_ID        133
#define WEBSITE_GROUP_ID        134

// OPTION_GROUP_ID controls
#define OPTION_INSTALL_PATH_LABEL_ID        200
#define OPTION_INSTALL_PATH_ENTRY_ID        201
#define OPTION_INSTALL_PATH_COMBO_ID        207
#define OPTION_LINK_PATH_LABEL_ID           203
#define OPTION_LINK_PATH_ENTRY_ID           204
#define OPTION_LINK_PATH_COMBO_ID           208
#define OPTION_SYMBOLIC_LINK_CHECK_ID       202
#define OPTION_OPTIONS_GROUP_ID             205
#define OPTION_GLOBAL_OPTIONS_GROUP_ID      209
#define OPTION_STATUS_LABEL_ID              206
#define OPTION_FREESPACE_LABEL_ID           213
#define OPTION_FREESPACE_VALUE_LABEL_ID     214
#define OPTION_ESTSIZE_LABEL_ID             215
#define OPTION_ESTSIZE_VALUE_LABEL_ID       216
#define OPTION_OPTIONS_SEPARATOR_ID         217
#define OPTION_CANCEL_BUTTON_ID             210
#define OPTION_README_BUTTON_ID             211
#define OPTION_CONTINUE_BUTTON_ID           212

// CLASS_GROUP_ID controls
#define CLASS_TEXT_LABEL_ID                 301
#define CLASS_RECOMMENDED_OPTION_ID         302
#define CLASS_EXPERT_OPTION_ID              303
#define CLASS_CANCEL_BUTTON_ID              304
#define CLASS_README_BUTTON_ID              305
#define CLASS_CONTINUE_BUTTON_ID            306

// COPY_GROUP_ID controls
#define COPY_TITLE_LABEL_ID                 400
#define COPY_CURRENT_FILE_LABEL_ID          401
#define COPY_CURRENT_FILE_PROGRESS_ID       405
#define COPY_TOTAL_LABEL_ID                 402
#define COPY_TOTAL_PROGRESS_ID              406
#define COPY_CANCEL_BUTTON_ID               403
#define COPY_README_BUTTON_ID               404

// DONE_GROUP_ID controls
#define DONE_INSTALL_DIR_LABEL_ID           503
#define DONE_GAME_LABEL_ID                  504
#define DONE_EXIT_BUTTON_ID                 505
#define DONE_README_BUTTON_ID               506
#define DONE_START_BUTTON_ID                507

// ABORT_GROUP_ID controls
#define ABORT_EXIT_BUTTON_ID                602

// WARNING_GROUP_ID controls
#define WARNING_TEXT_LABEL_ID               700
#define WARNING_CANCEL_BUTTON_ID            701
#define WARNING_CONTINUE_BUTTON_ID          702

// WEBSITE_GROUP_ID controls
#define WEBSITE_PRODUCT_LABEL_ID            801
#define WEBSITE_TEXT_LABEL_ID               802
#define WEBSITE_BROWSER_BUTTON_ID           804
#define WEBSITE_BROWSER_TEXT_ID             803
#define WEBSITE_CANCEL_BUTTON_ID            805
#define WEBSITE_README_BUTTON_ID            806
#define WEBSITE_CONTINUE_BUTTON_ID          807

#define LOKI_SETUP_SIG      'loki'

// Possible command events that are raised
#define COMMAND_EXIT        'exit'
#define COMMAND_CANCEL      'cncl'
#define COMMAND_CONTINUE    'cont'
#define COMMAND_README      'read'

// Different screens that we can display
typedef enum
{
    NONE_PAGE = -1,
	CLASS_PAGE = 0,
    OPTION_PAGE = 1,
    COPY_PAGE = 2,
    DONE_PAGE = 3,
    ABORT_PAGE = 4,
    WARNING_PAGE = 5,
    WEBSITE_PAGE = 6
} InstallPage;
// Number of pages that exist
#define PAGE_COUNT   7

typedef struct
{
    // Object references
    WindowRef Window;
    ControlRef PageHandles[PAGE_COUNT];

    int IsShown;
    InstallPage CurInstallPage;
    // Callback for application to handle command events (buttons)
    int (*CommandEventCallback)(UInt32);
} CarbonRes;

// Function declarations
CarbonRes *carbon_LoadCarbonRes(void (*CommandEventCallback)(UInt32));
void carbon_UnloadCarbonRes(CarbonRes *);
int carbon_IterateForState(int *);
void carbon_ShowInstallScreen(CarbonRes *, InstallPage);
void carbon_SetWindowTitle(CarbonRes *, char *);
void carbon_HideControl(CarbonRes *, int);
void carbon_DisableControl(CarbonRes *, int);
void carbon_EnableControl(CarbonRes *, int);
void carbon_SetInstallClass(CarbonRes *, int);
void carbon_UpdateImage(CarbonRes *, const char *, const char *);
void carbon_HandlePendingEvents();
void carbon_SetLabelText(CarbonRes *, int, const char *);
void carbon_SetProgress(CarbonRes *, int, float);

#endif