/*
void user_error_ptr() {}
void user_error_fn() {}
void user_warning_fn() {}

#define ERROR 1
u8 splash(void)
{
  char file_name[32] = "dslibris.png";
  FILE *fp = fopen(file_name, "rb");

  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)user_error_ptr,
     user_error_fn, user_warning_fn);
  if (!png_ptr)
    return (1);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
      png_destroy_read_struct(&png_ptr,
			      (png_infopp)NULL, (png_infopp)NULL);
      return (2);
    }
  
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
    {
      png_destroy_read_struct(&png_ptr, &info_ptr,
			      (png_infopp)NULL);
      return (3);
    }
  
  fclose(fp);
  return(0);
}
*/
