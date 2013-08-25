//============================================================================+
//
// $HeadURL: $
// $Revision: $
// $Date:  $
// $Author: $
//
/// \brief global variables
///
/// \file
///
// Changes
//
//============================================================================*/

/*--------------------------------- Definitions ------------------------------*/

#ifdef VAR_GLOBAL
#undef VAR_GLOBAL
#endif
#define VAR_GLOBAL extern

/*----------------------------------- Macros ---------------------------------*/

/*-------------------------------- Enumerations ------------------------------*/

/*------------------------------------ Types ---------------------------------*/

/*---------------------------------- Constants -------------------------------*/

/*----------------------------------- Globals --------------------------------*/

VAR_GLOBAL FATFS st_Fat;    //!< FAT object
VAR_GLOBAL FIL st_File;     //!< file object
VAR_GLOBAL bool b_FS_Ok;    //!< file system status
