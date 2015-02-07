

#define  E_SYMNMLEN  8   /* Number of characters in a symbol name         */
#define  E_FILNMLEN 14   /* Number of characters in a file name           */
#define  E_DIMNUM    4   /* Number of array dimensions in auxiliary entry */

 
/* Load a short int from the following tables with little-endian formats */
#define COFF_SHORT_L(ps) ((short)(((unsigned short)((unsigned char)ps[1])<<8)|\
				  ((unsigned short)((unsigned char)ps[0]))))

/* Load a long int from the following tables with little-endian formats */
#define COFF_LONG_L(ps) (((long)(((unsigned long)((unsigned char)ps[3])<<24) |\
				 ((unsigned long)((unsigned char)ps[2])<<16) |\
				 ((unsigned long)((unsigned char)ps[1])<<8)  |\
				 ((unsigned long)((unsigned char)ps[0])))))
 
/* Load a short int from the following tables with big-endian formats */
#define COFF_SHORT_H(ps) ((short)(((unsigned short)((unsigned char)ps[0])<<8)|\
				  ((unsigned short)((unsigned char)ps[1]))))

/* Load a long int from the following tables with big-endian formats */
#define COFF_LONG_H(ps) (((long)(((unsigned long)((unsigned char)ps[0])<<24) |\
				 ((unsigned long)((unsigned char)ps[1])<<16) |\
				 ((unsigned long)((unsigned char)ps[2])<<8)  |\
				 ((unsigned long)((unsigned char)ps[3])))))


#define COFF_LONG(v)   COFF_LONG_L(v)
#define COFF_SHORT(v)  COFF_SHORT_L(v)

/*** coff information for Intel 386/486.  */

/********************** FILE HEADER **********************/

struct COFF_filehdr {
	char f_magic[2];	/* magic number			*/
	char f_nscns[2];	/* number of sections		*/
	char f_timdat[4];	/* time & date stamp		*/
	char f_symptr[4];	/* file pointer to symtab	*/
	char f_nsyms[4];	/* number of symtab entries	*/
	char f_opthdr[2];	/* sizeof(optional hdr)		*/
	char f_flags[2];	/* flags			*/
};


#define  COFF_F_RELFLG		0000001
#define  COFF_F_EXEC		0000002
#define  COFF_F_LNNO		0000004
#define  COFF_F_LSYMS		0000010
#define  COFF_F_MINMAL		0000020
#define  COFF_F_UPDATE		0000040
#define  COFF_F_SWABD		0000100
#define  COFF_F_AR16WR		0000200
#define  COFF_F_AR32WR		0000400
#define  COFF_F_AR32W		0001000
#define  COFF_F_PATCH		0002000
#define  COFF_F_NODF		0002000

#define	COFF_I386MAGIC	        0x14c   /* Linux's system    */

#if 0   /* Perhaps, someday, these formats may be used.      */
#define COFF_I386PTXMAGIC	0x154
#define COFF_I386AIXMAGIC	0x175   /* IBM's AIX system  */
#define COFF_I386BADMAG(x) ((COFF_SHORT((x).f_magic) != COFF_I386MAGIC) \
			  && COFF_SHORT((x).f_magic) != COFF_I386PTXMAGIC \
			  && COFF_SHORT((x).f_magic) != COFF_I386AIXMAGIC)
#else
#define COFF_I386BADMAG(x) (COFF_SHORT((x).f_magic) != COFF_I386MAGIC)
#endif

#define	COFF_FILHDR	struct COFF_filehdr
#define	COFF_FILHSZ	sizeof(COFF_FILHDR)

/********************** AOUT "OPTIONAL HEADER" **********************/


typedef struct 
{
  char 	magic[2];		/* type of file				 */
  char	vstamp[2];		/* version stamp			 */
  char	tsize[4];		/* text size in bytes, padded to FW bdry */
  char	dsize[4];		/* initialized   data "   "		 */
  char	bsize[4];		/* uninitialized data "   "		 */
  char	entry[4];		/* entry pt.				 */
  char 	text_start[4];		/* base of text used for this file       */
  char 	data_start[4];		/* base of data used for this file       */
}
COFF_AOUTHDR;

#define COFF_AOUTSZ (sizeof(COFF_AOUTHDR))

#define COFF_STMAGIC	0401
#define COFF_OMAGIC     0404
#define COFF_JMAGIC     0407    /* dirty text and data image, can't share  */
#define COFF_DMAGIC     0410    /* dirty text segment, data aligned        */
#define COFF_ZMAGIC     0413    /* The proper magic number for executables  */
#define COFF_SHMAGIC	0443	/* shared library header                   */

/********************** SECTION HEADER **********************/

struct COFF_scnhdr {
  char		s_name[8];	/* section name			    */
  char		s_paddr[4];	/* physical address, aliased s_nlib */
  char		s_vaddr[4];	/* virtual address		    */
  char		s_size[4];	/* section size			    */
  char		s_scnptr[4];	/* file ptr to raw data for section */
  char		s_relptr[4];	/* file ptr to relocation	    */
  char		s_lnnoptr[4];	/* file ptr to line numbers	    */
  char		s_nreloc[2];	/* number of relocation entries	    */
  char		s_nlnno[2];	/* number of line number entries    */
  char		s_flags[4];	/* flags			    */
};

#define	COFF_SCNHDR	struct COFF_scnhdr
#define	COFF_SCNHSZ	sizeof(COFF_SCNHDR)


#define COFF_TEXT	".text"
#define COFF_DATA	".data"
#define COFF_BSS	".bss"
#define COFF_COMMENT    ".comment"
#define COFF_LIB        ".lib"

#define COFF_SECT_TEXT  0      /* Section for instruction code             */
#define COFF_SECT_DATA  1      /* Section for initialized globals          */
#define COFF_SECT_BSS   2      /* Section for un-initialized globals       */
#define COFF_SECT_REQD  3      /* Minimum number of sections for good file */

#define COFF_STYP_REG     0x00 /* regular segment                          */
#define COFF_STYP_DSECT   0x01 /* dummy segment                            */
#define COFF_STYP_NOLOAD  0x02 /* no-load segment                          */
#define COFF_STYP_GROUP   0x04 /* group segment                            */
#define COFF_STYP_PAD     0x08 /* .pad segment                             */
#define COFF_STYP_COPY    0x10 /* copy section                             */
#define COFF_STYP_TEXT    0x20 /* .text segment                            */
#define COFF_STYP_DATA    0x40 /* .data segment                            */
#define COFF_STYP_BSS     0x80 /* .bss segment                             */
#define COFF_STYP_INFO   0x200 /* .comment section                         */
#define COFF_STYP_OVER   0x400 /* overlay section                          */
#define COFF_STYP_LIB    0x800 /* library section                          */


struct COFF_slib {
  char		sl_entsz[4];	/* Size of this entry               */
  char		sl_pathndx[4];	/* size of the header field         */
};

#define	COFF_SLIBHD	struct COFF_slib
#define	COFF_SLIBSZ	sizeof(COFF_SLIBHD)

/********************** LINE NUMBERS **********************/


struct COFF_lineno {
  union {
    char l_symndx[4];	/* function name symbol index, iff l_lnno == 0*/
    char l_paddr[4];	/* (physical) address of line number	*/
  } l_addr;
  char l_lnno[2];	/* line number		*/
};

#define	COFF_LINENO	struct COFF_lineno
#define	COFF_LINESZ	6

/********************** SYMBOLS **********************/

#define COFF_E_SYMNMLEN	 8	/* # characters in a short symbol name	*/
#define COFF_E_FILNMLEN	14	/* # characters in a file name		*/
#define COFF_E_DIMNUM	 4	/* # array dimensions in auxiliary entry */


struct COFF_syment 
{
  union {
    char e_name[E_SYMNMLEN];    /* Symbol name (first 8 characters) */
    struct {
      char e_zeroes[4];         /* Leading zeros */
      char e_offset[4];         /* Offset if this is a header section */
    } e;
  } e;

  char e_value[4];              /* Value (address) of the segment */
  char e_scnum[2];              /* Section number */
  char e_type[2];               /* Type of section */
  char e_sclass[1];             /* Loader class */
  char e_numaux[1];             /* Number of auxiliary entries which follow */
};

#define COFF_N_BTMASK	(0xf)   /* Mask for important class bits */
#define COFF_N_TMASK	(0x30)  /* Mask for important type bits  */
#define COFF_N_BTSHFT	(4)     /* # bits to shift class field   */
#define COFF_N_TSHIFT	(2)     /* # bits to shift type field    */

  
union COFF_auxent {


  struct {
    char x_tagndx[4];	        /* str, un, or enum tag indx */
    union {
      struct {
	char  x_lnno[2];        /* declaration line number */
	char  x_size[2];        /* str/union/array size */
      } x_lnsz;
      char x_fsize[4];	        /* size of function */
    } x_misc;

    union {
      struct {		        /* if ISFCN, tag, or .bb */
	char x_lnnoptr[4];	/* ptr to fcn line # */
	char x_endndx[4];	/* entry ndx past block end */
      } x_fcn;

      struct {		        /* if ISARY, up to 4 dimen. */
	char x_dimen[E_DIMNUM][2];
      } x_ary;
    } x_fcnary;

    char x_tvndx[2];	/* tv index */
  } x_sym;


  union {
    char x_fname[E_FILNMLEN];
    struct {
      char x_zeroes[4];
      char x_offset[4];
    } x_n;
  } x_file;


  struct {
    char x_scnlen[4];	/* section length */
    char x_nreloc[2];	/* # relocation entries */
    char x_nlinno[2];	/* # line numbers */
  } x_scn;

  
  struct {
    char x_tvfill[4];	/* tv fill value */
    char x_tvlen[2];	/* length of .tv */
    char x_tvran[2][2];	/* tv range */
  } x_tv;		/* info about .tv section (in auxent of symbol .tv)) */
};

#define	COFF_SYMENT	struct COFF_syment
#define	COFF_SYMESZ	18	
#define	COFF_AUXENT	union COFF_auxent
#define	COFF_AUXESZ	18

#define COFF_ETEXT	"etext"

/********************** RELOCATION DIRECTIVES **********************/

struct COFF_reloc {
  char r_vaddr[4];        /* Virtual address of item    */
  char r_symndx[4];       /* Symbol index in the symtab */
  char r_type[2];         /* Relocation type            */
};

#define COFF_RELOC struct COFF_reloc
#define COFF_RELSZ 10

#define COFF_DEF_DATA_SECTION_ALIGNMENT  4
#define COFF_DEF_BSS_SECTION_ALIGNMENT   4
#define COFF_DEF_TEXT_SECTION_ALIGNMENT  4

/* For new sections we haven't heard of before */
#define COFF_DEF_SECTION_ALIGNMENT       4
