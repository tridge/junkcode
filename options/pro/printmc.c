/* Prints a copy of the spreadsheet to a file or to the printer
{
 char filename[MAXINPUT + 1], s[133], colstr[MAXCOLWIDTH + 1];
 FILE *file;
 int columns, counter1, counter2, counter3, col = 0, row, border, toppage,
  lcol, lrow, dummy, printed, oldlastcol;

 filename[0] = 0;
 writeprompt(MSGPRINT);
 if (!editstring(filename, "", MAXINPUT))
  return;
 if (filename[0] == 0)
  strcpy(filename, "PRN");
 if ((file = fopen(filename, "wt")) == NULL)
 {
  errormsg(MSGNOOPEN);
  return;
 }
 oldlastcol = lastcol;
 for (counter1 = 0; counter1 <= lastrow; counter1++)
 {
  for (counter2 = lastcol; counter2 < MAXCOLS; counter2++)
  {
   if (format[counter2][counter1] >= OVERWRITE)
    lastcol = counter2;
  }
 }
 if (!getyesno(&columns, MSGCOLUMNS))
  return;
 columns = (columns == 'Y') ? 131 : 79;
 if (!getyesno(&border, MSGBORDER))
  return;
 border = (border == 'Y');
 while (col <= lastcol)
 {
  row = 0;
  toppage = TRUE;
  lcol = pagecols(col, border, columns) + col;
  while (row <= lastrow)
  {
   lrow = pagerows(row, toppage, border) + row;
   printed = 0;
   if (toppage)
   {
    for (counter1 = 0; counter1 < TOPMARGIN; counter1++)
    {
     fprintf(file, "\n");
     printed++;
    }
   }
   for (counter1 = row; counter1 < lrow; counter1++)
   {
    if ((border) && (counter1 == row) && (toppage))
    {
     if ((col == 0) && (border))
      sprintf(s, "%*s", LEFTMARGIN, "");
     else
      s[0] = 0;
     for (counter3 = col; counter3 < lcol; counter3++)
     {
      centercolstring(counter3, colstr);
      strcat(s, colstr);
     }
     fprintf(file, "%s\n", s);
     printed++;
    }
    if ((col == 0) && (border))
     sprintf(s, "%-*d", LEFTMARGIN, counter1 + 1);
    else
     s[0] = 0;
    for (counter2 = col; counter2 < lcol; counter2++)
     strcat(s, cellstring(counter2, counter1, &dummy, FORMAT));
    fprintf(file, "%s\n", s);
    printed++;
   }
   row = lrow;
   toppage = FALSE;
   if (printed < 66)
    fprintf(file, "%c", FORMFEED);
  }
  col = lcol;
 }
 fclose(file);
 lastcol = oldlastcol;
}  printsheet */
