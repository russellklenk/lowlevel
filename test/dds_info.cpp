/*/////////////////////////////////////////////////////////////////////////////
/// @summary Loads and displays information about DDS files. 
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <stdio.h>
#include <stdlib.h>
#include "lldatain.hpp"

/*/////////////////
//   Constants   //
/////////////////*/

/*///////////////////////
//   Local Functions   //
///////////////////////*/

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Implements the entry point of the application.
/// @param argc The number of arguments passed on the command line.
/// @param argv An array of strings specifying command line arguments.
/// @return EXIT_SUCCESS or EXIT_FAILURE.
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("ERROR: Not enough command-line arguments.\n");
        printf("USAGE: ddsinfo path/to/file.dds\n");
        exit(EXIT_FAILURE);
    }

    size_t dds_size = 0;
    void  *dds_data = data::load_binary(argv[1], &dds_size);
    if (dds_data == NULL)
    {
        printf("ERROR: Input file \'%s\' not found.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("INFO: Loaded \'%s\', %u bytes.\n", argv[1], uint32_t(dds_size));
    }

    size_t                    count     =  0;
    size_t                   nitems     =  0;
    size_t                   nlevels    =  0;
    data::dds_level_desc_t   *levels    = NULL;
    data::dds_header_dxt10_t *h_ex      = NULL;
    data::dds_header_t        header    = {0};
    data::dds_header_dxt10_t  header_ex = {0};
    if (!data::dds_header(dds_data, dds_size, &header))
    {
        printf("ERROR: File does not appear to be a valid DDS.\n");
        goto cleanup_error;
    }
    // @note: need to change the API so it looks at the FourCC.
    // clients can't be relied upon to create and manage a separate pointer,
    // and doing the logical thing will result in incorrect behavior as-is.
    if (!data::dds_header_dxt10(dds_data, dds_size, &header_ex))
    {
        printf("INFO: No extended header present.\n"); 
        h_ex = NULL;
    }
    else
    {
        printf("INFO: Found extended header.\n");
        h_ex = &header_ex;
    }

    nitems   = data::dds_array_count(&header, h_ex);
    nlevels  = data::dds_level_count(&header, h_ex);
    if (nitems == 0 && nlevels == 0)
    {
        printf("ERROR: File appears invalid; no items or levels.\n");
        goto cleanup_error;
    }
    else
    {
        printf("INFO: Found %u surface(s), (each) with %u levels.\n", uint32_t(nitems), uint32_t(nlevels));
    }

    levels = (data::dds_level_desc_t*) malloc(nitems * nlevels * sizeof(data::dds_level_desc_t));
    count  =  data::dds_describe(dds_data, dds_size, &header, h_ex, levels, nitems * nlevels);
    if (count == 0)
    {
        printf("ERROR: Failed to describe the surface(s).\n");
        goto cleanup_error;
    }
    else
    {
        printf("INFO: Described %u/%u level(s).\n", uint32_t(count), uint32_t(nlevels * nitems));
    }

    free(levels);
    free(dds_data);
    exit(EXIT_SUCCESS);

cleanup_error:
    if (levels) free(levels);
    if (dds_data) free(dds_data);
    exit(EXIT_FAILURE);
}

