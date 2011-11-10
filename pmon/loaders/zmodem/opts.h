/*
 * program argument parsing macros
 */

#define OPT_STRING(l,p) case l: { p = s+1;       *s = 0; s-- ; break; }
#define OPT_BOOL(l,p)   case l: { p = TRUE;                    break; }
#define OPT_NUM(l,p)    case l: { p = atoi(s+1); *s = 0; s-- ; break; }
