/*
 * "dialog"-based UI frontend for setup.
 * Dialog was turned into a library, shell commands are not called.
 *
 * $Id: dialog_ui.c,v 1.5 2002-10-19 07:41:09 megastep Exp $
 */

#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "install.h"
#include "install_ui.h"
#include "detect.h"
#include "file.h"
#include "copy.h"
#include "loki_launchurl.h"
#include "dialog/dialog.h"


/* Count the number of newlines in a string */
static int count_lines(const char *str)
{
    int ret = 1; /* At least one line */
    while ( *str ) {
	if ( *str ++ == '\n' )
	    ++ret;
    }
    return ret;
}

static void clear_screen(void)
{
	dialog_clear();
	put_backtitle();
}

static
yesno_answer dialog_prompt(const char *txt, yesno_answer suggest)
{
    if ( suggest == RESPONSE_OK ) {
	dialog_msgbox(_("Message"), txt, count_lines(txt)+5, 50, 1);
	return RESPONSE_OK;
    } else {
	int ret = dialog_yesno(_("Request"), txt, count_lines(txt)+5, 50,
			       suggest == RESPONSE_NO);
	return (ret == 0) ? RESPONSE_YES : RESPONSE_NO;
    }
}

static
install_state dialog_init(install_info *info,int argc, char **argv, 
			  int noninteractive)
{
    static char title[BUFSIZ];
	char msg[BUFSIZ];

    if ( info->component ) {
		snprintf(title, sizeof(title),
				 _("%s / %s Installation"), info->desc,
				 GetProductComponent(info));
    } else {
		snprintf(title, sizeof(title),
				 _("%s Installation"), info->desc);
    }
	
	dialog_vars.cr_wrap = TRUE;
	dialog_vars.tab_correct = FALSE;

	dialog_vars.backtitle = title;

    snprintf(msg, sizeof(msg),
			 _("You are running a %s machine with %s\nDistribution: %s %d.%d"),
			 info->arch, info->libc, distribution_name[info->distro],
			 info->distro_maj, info->distro_min);
	
    /* Welcome box */
	clear_screen();
    dialog_msgbox(title, msg, 6, 50, 1);

    info->install_size = size_tree(info, info->config->root->childs);

    if ( GetProductEULA(info) ) {
        return SETUP_LICENSE;
    } else {
        return SETUP_README;
    }
}

static
install_state dialog_license(install_info *info)
{
    dialog_textbox(_("License Agreement"), GetProductEULA(info),
				   -1, -1);
	clear_screen();
    if ( dialog_prompt(_("Do you agree with the license?"), RESPONSE_YES) ==
	                  RESPONSE_YES ) {
        return SETUP_README;
    } else {
        return SETUP_ABORT;
    }
}

static
install_state dialog_readme(install_info *info)
{
    const char *readme;

    readme = GetProductREADME(info);
    if ( readme ) {
        char prompt[256];

	clear_screen();
	readme += strlen(info->setup_path)+1; /* Skip the install path */
        snprintf(prompt, sizeof(prompt), _("Would you like to read the %s file ?"), readme);
        if ( dialog_prompt(prompt, RESPONSE_YES) == RESPONSE_YES ) {
			dialog_textbox(_("README"), GetProductREADME(info),
						   -1, -1);
        }
    }
	if ( GetProductAllowsExpress(info) ) {
		return SETUP_CLASS;
	} else {
 		return SETUP_OPTIONS;
	}
}

#define MAX_CHOICES 16

/* This function returns a boolean value that tells if parsing for this node's siblings
   should continue (used for exclusive options) */
static int parse_option(install_info *info, const char *component, xmlNodePtr parent, int exclusive,
						const char *text)
{
	const char *choices[MAX_CHOICES*4];
	char buf[BUFSIZ];
	int result[MAX_CHOICES];
	xmlNodePtr nodes[MAX_CHOICES];
	int i = 0, nb_choices = 0;

	xmlNodePtr node = parent->childs;
	for ( ; node && nb_choices<MAX_CHOICES; node = node->next ) {
	    const char *set = xmlGetProp(node, "install");

		if ( ! set ) {
			/* it's also possible to use the "required" attribute */
			set = xmlGetProp(node, "required");
		}
	    if ( ! strcmp(node->name, "option") ) {
		    
			if ( ! match_arch(info, xmlGetProp(node, "arch")) )
				continue;
		
			if ( ! match_libc(info, xmlGetProp(node, "libc")) )
				continue;
			 
			if ( ! match_distro(info, xmlGetProp(node, "distro")) )
				continue;
		
			if ( ! get_option_displayed(info, node) ) {
				continue;
			}

			/* Skip options that are already installed */
			if ( info->product && ! GetProductReinstall(info) ) {
				product_component_t *comp;
				
				if ( component ) {
					comp = loki_find_component(info->product, component);
				} else {
					comp = loki_getdefault_component(info->product);
				}
				if ( comp && loki_find_option(comp, get_option_name(info, node, NULL, 0)) ) {
					continue;
				}
			}
			nodes[nb_choices] = node;
			choices[i++] = "";
			choices[i++] = strdup(get_option_name(info, node, NULL, 0));
			choices[i++] = (set && !strcmp(set,"true")) ? "on" : "off";
			choices[i++] = get_option_help(info, node);

			nb_choices++;
	    } else if ( ! strcmp(node->name, "exclusive") ) {
			nodes[nb_choices] = node;
			choices[i++] = "";
			choices[i++] = strdup(get_option_name(info, node, NULL, 0));
			choices[i++] = "on";
			choices[i++] = get_option_help(info, node);
			nb_choices++;
	    } else if ( ! strcmp(node->name, "component") ) {
			nodes[nb_choices] = node;
			choices[i++] = _("Component");
			choices[i++] = xmlGetProp(node, "name");
			choices[i++] = (set && !strcmp(set,"true")) ? "on" : "off";
			choices[i++] = get_option_help(info, node);
			nb_choices++;
	    }
	}


	/* TTimo: loop until the selected options are all validated by possible EULA */
options_loop:  
	clear_screen();
	snprintf(buf, sizeof(buf), _("Installation of %s"), info->desc); 
	if ( dialog_checklist(buf, text,
						  7+nb_choices, COLS*9/10, nb_choices, nb_choices, choices, 
						  !exclusive, result) != DLG_EXIT_OK) {
		return 0;
	}

	/* Now parse the results */
	for ( i = 0; i < nb_choices; ++i ) {
		if ( !strcmp(nodes[i]->name, "option") ) {
		    if ( result[i] ) {
				const char *warn = get_option_warn(info, nodes[i]);

				/* does this option have a EULA item */
				xmlNodePtr child;
				child = nodes[i]->childs;
				while(child) {
					if (!strcmp(child->name, "eula")) {
						/* this option has some EULA nodes
						 * we need to prompt before this change can be validated / turned on
						 */
						const char* name = GetProductEULANode(info, nodes[i]);
						if (name) {
							/* prompt for the EULA */
							dialog_textbox(_("License Agreement"), name, -1, -1);
							clear_screen();
							if ( dialog_prompt(_("Do you agree with the license?"), RESPONSE_YES) == RESPONSE_NO ) {
								/* refused the license, don't set the option and bounce back */
								choices[i*4+2] = "off";
								goto options_loop; /* I hate doing this :( */
							}							
						} else {
							snprintf(buf, BUFSIZ, _("Option-specific EULA not found for '%s', can't toggle\n"), choices[i*4+1]);
							dialog_msgbox(_("Error"), buf, 6, strlen(buf), 1);
							choices[i*4+2] = "off";
							goto options_loop;
						}
					}
					child = child->next;
				}			
				
				if ( warn ) { /* Display a warning message to the user */
					snprintf(buf, sizeof(buf), "%s:\n%s", get_option_name(info, nodes[i], NULL, 0), warn);
					dialog_prompt(buf, RESPONSE_OK);
				}
				/* Mark this option for installation */
				mark_option(info, nodes[i], "true", 0);
				
				/* Add this option size to the total */
				info->install_size += size_node(info, nodes[i]);
		    } else if ( xmlGetProp(nodes[i], "required") ) {
				snprintf(buf, sizeof(buf), _("Option '%s' is required.\n"), 
						 get_option_name(info, nodes[i], NULL, 0));
				dialog_prompt(buf, RESPONSE_OK);
			    
				/* Mark this option for installation */
				mark_option(info, nodes[i], "true", 0);
				
				/* Add this option size to the total */
				info->install_size += size_node(info, nodes[i]);
		    } else { /* Unmark */
				mark_option(info, nodes[i], "false", 0);
		    }
		} else if ( !strcmp(nodes[i]->name, "exclusive") && result[i]) {
		    parse_option(info, component, nodes[i], 1, get_option_name(info, nodes[i], NULL, 0));
		} else if ( !strcmp(nodes[i]->name, "component") && result[i]) {
		    parse_option(info, xmlGetProp(nodes[i], "name"), nodes[i], 1, xmlGetProp(nodes[i], "name"));
		}
	}
	return 1;
}

static
install_state dialog_setup(install_info *info)
{
    xmlNodePtr node;
	const char *choices[MAX_CHOICES];
	char buf[BUFSIZ];
	int i = 0, choice;

	if ( ! express_setup ) {

		/* Prompt for all options */

		clear_screen();
		if ( GetProductIsMeta(info) ) {
			const char *wanted;

			/* Build a list of products */
			node = info->config->root->childs;
			while ( node ) {
				if ( strcmp(node->name, "option") == 0 ) {
					choices[i] = get_option_name(info, node, NULL, 0);
					wanted = xmlGetProp(node, "install"); /* Look for a default value */
					if ( wanted && !strcmp(wanted, "true") ) {
						dialog_vars.default_item = choices[i];
					}
					i++;
				}
				node = node->next;
			}
			choice = dialog_menu(_("Choose the product"), 
								 _("Please pick a product to install"),
								 i + 6, COLS*8/10, i, i, choices);
			if ( choice < 0 ) {
				return SETUP_ABORT;
			} else {
				/* TODO: Process the choice */
			}
		} else {
			char path[PATH_MAX];
			int okay = 0;

			while ( !okay ) {
				if ( !disable_install_path ) {
					if ( dialog_inputbox(_("Installation path"), 
										 _("Please enter the installation path"),
										 10, 50, info->install_path, 0) < 0 ) {
						return SETUP_ABORT;
					}
					strncpy(path, dialog_vars.input_result, sizeof(path));
				} else {
					strcpy(path, info->install_path);
				}
				set_installpath(info, path);

				/* Check permissions on the install path */
				topmost_valid_path(path, info->install_path);

				if ( access(path, F_OK) < 0 ) {
					if ( (strcmp(path, "/") != 0) && strrchr(path, '/') ) {
						*(strrchr(path, '/')+1) = '\0';
					}
				}
		
				if ( ! dir_is_accessible(path) ) {
					snprintf(buf, sizeof(buf), _("No write permission to %s\n"), path);
					if ( dialog_msgbox(_("Failed permissions"), buf, 10, strlen(buf)+6, 1)==0
						 &&  ! disable_install_path ) {
						continue;
					} else {
						return SETUP_ABORT;
					}
				}

				/* Default to empty */
				path[0] = '\0';

				/* Find out where to install the binary symlinks, unless the binary path
				   was provided as a command line argument */
				if ( ! disable_binary_path ) {
					int get_path = 1;

					if (GetProductHasNoBinaries(info))
						get_path = 0;

					if (get_path) {
						/*----------------------------------------
						**  Optionally, ask the user whether
						**    they want to create a symlink
						**    to the path or not.
						**--------------------------------------*/
						if (GetProductHasPromptBinaries(info)) {
							if (dialog_prompt(_("Do you want to install symbolic links to a directory in your path?"), RESPONSE_YES) != RESPONSE_YES) {
								get_path = 0;
							}
						}
					}
				
					if (get_path) {
						clear_screen();
						if ( dialog_inputbox(_("Symlink path"), 
											 _("Please enter the path in which to create the symbolic links"),
											 10, 50, info->symlinks_path, 0) < 0 ) {
							return SETUP_ABORT;
						} else {
							strncpy(path, dialog_vars.input_result, sizeof(path));
						}
					}
				} else {
					strcpy(path, info->symlinks_path);
				}

				set_symlinkspath(info, path);

				/* If the binary and install path are the same, give an error */
				if (strcmp(info->symlinks_path, info->install_path) == 0) {
					dialog_msgbox(_("Error"), _("Binary path must be different than the install path.\nThe binary path must be an existing directory.\n"), 10, 40, 1);
					continue;
				}

				/* Check permissions on the symlinks path, if the path was 
				   provided as a command-line argument and it is invalid, then
				   abort  */
				if ( *info->symlinks_path ) {
					if ( access(info->symlinks_path, W_OK) < 0 ) {
						snprintf(buf, sizeof(buf), _("No write permission to %s\n"), 
								 info->symlinks_path);
					
						if (dialog_msgbox(_("Error"), buf, 10, 50, 1)==0 && 
							! disable_binary_path) {
							continue;
						} else {
							return SETUP_ABORT;
						}
					}
				}

				/* Go through the install options */

				info->install_size = 0;
				if ( parse_option(info, NULL, info->config->root, 0, _("Please choose the options to install")) ) {
					okay = TRUE;
				} else {
					return SETUP_ABORT;
				}

				clear_screen();
				/* Ask for desktop menu items */
				if ( !GetProductHasNoBinaries(info) &&
					 dialog_prompt(_("Do you want to install startup menu entries?"),
								   RESPONSE_YES) == RESPONSE_YES ) {
					info->options.install_menuitems = 1;
				}

			} /* while okay */

		}
	} else { /* Express setup */
		/* Install desktop menu items */
		if ( !GetProductHasNoBinaries(info)) {
			info->options.install_menuitems = 1;
		}
	}
	dialog_vars.default_item = NULL;

	clear_screen();
	dialog_gauge_begin(10, COLS*8/10, 0);
    return SETUP_INSTALL;
}

static
int dialog_update(install_info *info, const char *path, size_t progress, 
				  size_t size, const char *current)
{
	char buf[PATH_MAX];
    static char previous[200] = "";

    if(strcmp(previous, current)){
        strncpy(previous,current, sizeof(previous));
		/* FIXME: Anything useful to be done here? */
	}

	if ( progress && size ) {
        int percent = (int) (((float)progress/(float)size)*100.0);
		snprintf(buf, sizeof(buf), _("Installing %s ..."), current);
		dialog_gauge_update(_("Installing..."), buf, percent);
    } else { /* Script */
		dialog_gauge_update(_("Installing..."), path, 100);
	}
	return 1;
}

static
void dialog_exit(install_info *info)
{
    end_dialog();
}

static
void dialog_abort(install_info *info)
{
	clear_screen();
	dialog_msgbox(_("Aborting"), _("Install aborted - cleaning up files\n"),
				  3, 45, 0);
	dialog_exit(info);
}

static
install_state dialog_website(install_info *info)
{
	clear_screen();
    if ( (strcmp( GetAutoLaunchURL(info), "true" )==0)
         || (dialog_prompt(_("Would you like to launch a Web browser?"), RESPONSE_YES) == RESPONSE_YES ) ) {
        launch_browser(info, loki_launchURL);
    }
    return SETUP_COMPLETE;
}

static
install_state dialog_complete(install_info *info)
{
    install_state new_state = SETUP_EXIT;

	dialog_gauge_end();
	clear_screen();
    if ( info->installed_symlink && *info->play_binary &&
         dialog_prompt(_("Installation complete!\nWould you like to start now?"), 
					   RESPONSE_YES) == RESPONSE_YES ) {
		new_state = SETUP_PLAY;
	} else {
		dialog_msgbox(_("Finished"), _("Installation complete!"), 3, 45, 0);
	}
    return new_state;
}

static
install_state dialog_pick_class(install_info *info)
{
	const char *choices[8] = {
		_("Recommended"), "", "on", _("Express installation using detected defaults"),
		_("Expert"), "", "off", _("Full control over the installation parameters")
	};
	const char *msg;
	int result[2];
	char buf[BUFSIZ];

	/* FIXME: Catch errors and select expert when possible */
	msg = IsReadyToInstall(info);
	if ( msg ) { /* Select "Expert" by default */
		choices[2] = "off";
		choices[6] = "on";
	}

	clear_screen();
	snprintf(buf, sizeof(buf), _("Installation of %s"), info->desc); 
	if ( dialog_checklist(buf, _("Please select the class of installation"),
						  9, COLS*9/10, 2, 2, choices,
						  0, result) != DLG_EXIT_OK) {
		return SETUP_ABORT;
	}

	express_setup = result[0];
	if ( express_setup && msg ) {
		snprintf(buf, sizeof(buf),
				 _("Installation could not proceed due to the following error:\n%s\nTry to use 'Expert' installation."), 
				 msg);
		dialog_prompt(buf, RESPONSE_OK);
		return SETUP_CLASS;
	}
	return SETUP_OPTIONS;
}

static void dialog_shutdown(install_info *info)
{
    clear_screen();
    end_dialog();
}

int dialog_okay(Install_UI *UI, int *argc, char ***argv)
{
    if ( !isatty(STDIN_FILENO) )
	return(0);
    if ( !init_dialog() )
	return(0);

    /* Set up the driver */
    UI->init = dialog_init;
    UI->license = dialog_license;
    UI->readme = dialog_readme;
    UI->setup = dialog_setup;
    UI->update = dialog_update;
    UI->abort = dialog_abort;
    UI->prompt = dialog_prompt;
    UI->website = dialog_website;
    UI->complete = dialog_complete;
    UI->exit = dialog_exit;
    UI->pick_class = dialog_pick_class;
    UI->idle = NULL;
    UI->shutdown = dialog_shutdown;
    UI->is_gui = 0;

    return(1);
}



