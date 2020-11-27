/*******************************************************************************************************************//**
 * @file    utilities.c
 * @author  G.Zini
 * @version 1.0
 * @date    2018 July, 24
 * @brief   General purpose utility functions
 **********************************************************************************************************************/

// Includes ------------------------------------------------------------------------------------------------------------
#include "utilities.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


// Private macro -------------------------------------------------------------------------------------------------------

// Test and convertion of an ASCII char to an hex nibble
#define _tohex(ch) (((uint8_t)((ch)-='0') <= 9u) \
                || (((uint8_t)((ch)-=('A'-'0'-10u)) >= 10u) && (ch <= 15u)) \
                || (((uint8_t)((ch)-=('a'-'A'))     >= 10u) && (ch <= 15u)))

// Test and convertion of an ASCII char to a decimal nibble
#define _todec(ch) ((uint8_t)(ch -= '0') < 10u)


// Private variables ---------------------------------------------------------------------------------------------------

// Function 32767*sin(x) table. The table has 1024+1 entries ranging from 0 to 90 degrees
static const int32_t sinTab[] =
{
    0,50,101,151,201,251,302,352,402,452,503,553,603,653,704,754,804,854,905,
    955,1005,1055,1106,1156,1206,1256,1307,1357,1407,1457,1507,1558,1608,1658,
    1708,1758,1809,1859,1909,1959,2009,2059,2110,2160,2210,2260,2310,2360,2410,
    2461,2511,2561,2611,2661,2711,2761,2811,2861,2911,2962,3012,3062,3112,3162,
    3212,3262,3312,3362,3412,3462,3512,3562,3612,3662,3712,3761,3811,3861,3911,
    3961,4011,4061,4111,4161,4210,4260,4310,4360,4410,4460,4509,4559,4609,4659,
    4708,4758,4808,4858,4907,4957,5007,5056,5106,5156,5205,5255,5305,5354,5404,
    5453,5503,5552,5602,5651,5701,5750,5800,5849,5899,5948,5998,6047,6096,6146,
    6195,6245,6294,6343,6393,6442,6491,6540,6590,6639,6688,6737,6786,6836,6885,
    6934,6983,7032,7081,7130,7179,7228,7277,7326,7375,7424,7473,7522,7571,7620,
    7669,7718,7767,7815,7864,7913,7962,8010,8059,8108,8157,8205,8254,8303,8351,
    8400,8448,8497,8545,8594,8642,8691,8739,8788,8836,8885,8933,8981,9030,9078,
    9126,9175,9223,9271,9319,9367,9416,9464,9512,9560,9608,9656,9704,9752,9800,
    9848,9896,9944,9992,10039,10087,10135,10183,10231,10278,10326,10374,10421,
    10469,10517,10564,10612,10659,10707,10754,10802,10849,10897,10944,10992,
    11039,11086,11133,11181,11228,11275,11322,11370,11417,11464,11511,11558,
    11605,11652,11699,11746,11793,11840,11886,11933,11980,12027,12074,12120,
    12167,12214,12260,12307,12353,12400,12446,12493,12539,12586,12632,12679,
    12725,12771,12817,12864,12910,12956,13002,13048,13094,13141,13187,13233,
    13279,13324,13370,13416,13462,13508,13554,13599,13645,13691,13736,13782,
    13828,13873,13919,13964,14010,14055,14101,14146,14191,14236,14282,14327,
    14372,14417,14462,14507,14553,14598,14643,14688,14732,14777,14822,14867,
    14912,14956,15001,15046,15090,15135,15180,15224,15269,15313,15358,15402,
    15446,15491,15535,15579,15623,15667,15712,15756,15800,15844,15888,15932,
    15976,16019,16063,16107,16151,16195,16238,16282,16325,16369,16413,16456,
    16499,16543,16586,16630,16673,16716,16759,16802,16846,16889,16932,16975,
    17018,17061,17104,17146,17189,17232,17275,17317,17360,17403,17445,17488,
    17530,17573,17615,17657,17700,17742,17784,17827,17869,17911,17953,17995,
    18037,18079,18121,18163,18204,18246,18288,18330,18371,18413,18454,18496,
    18537,18579,18620,18661,18703,18744,18785,18826,18868,18909,18950,18991,
    19032,19072,19113,19154,19195,19236,19276,19317,19357,19398,19438,19479,
    19519,19560,19600,19640,19680,19721,19761,19801,19841,19881,19921,19961,
    20000,20040,20080,20120,20159,20199,20238,20278,20317,20357,20396,20436,
    20475,20514,20553,20592,20631,20670,20709,20748,20787,20826,20865,20904,
    20942,20981,21019,21058,21096,21135,21173,21212,21250,21288,21326,21364,
    21403,21441,21479,21516,21554,21592,21630,21668,21705,21743,21781,21818,
    21856,21893,21930,21968,22005,22042,22079,22116,22154,22191,22227,22264,
    22301,22338,22375,22411,22448,22485,22521,22558,22594,22631,22667,22703,
    22739,22776,22812,22848,22884,22920,22956,22991,23027,23063,23099,23134,
    23170,23205,23241,23276,23311,23347,23382,23417,23452,23487,23522,23557,
    23592,23627,23662,23697,23731,23766,23801,23835,23870,23904,23938,23973,
    24007,24041,24075,24109,24143,24177,24211,24245,24279,24312,24346,24380,
    24413,24447,24480,24514,24547,24580,24613,24647,24680,24713,24746,24779,
    24811,24844,24877,24910,24942,24975,25007,25040,25072,25105,25137,25169,
    25201,25233,25265,25297,25329,25361,25393,25425,25456,25488,25519,25551,
    25582,25614,25645,25676,25708,25739,25770,25801,25832,25863,25893,25924,
    25955,25986,26016,26047,26077,26108,26138,26168,26198,26229,26259,26289,
    26319,26349,26378,26408,26438,26468,26497,26527,26556,26586,26615,26644,
    26674,26703,26732,26761,26790,26819,26848,26876,26905,26934,26962,26991,
    27019,27048,27076,27104,27133,27161,27189,27217,27245,27273,27300,27328,
    27356,27384,27411,27439,27466,27493,27521,27548,27575,27602,27629,27656,
    27683,27710,27737,27764,27790,27817,27843,27870,27896,27923,27949,27975,
    28001,28027,28053,28079,28105,28131,28157,28182,28208,28234,28259,28284,
    28310,28335,28360,28385,28411,28436,28460,28485,28510,28535,28560,28584,
    28609,28633,28658,28682,28706,28730,28755,28779,28803,28827,28850,28874,
    28898,28922,28945,28969,28992,29016,29039,29062,29085,29108,29131,29154,
    29177,29200,29223,29246,29268,29291,29313,29336,29358,29380,29403,29425,
    29447,29469,29491,29513,29534,29556,29578,29599,29621,29642,29664,29685,
    29706,29728,29749,29770,29791,29812,29832,29853,29874,29894,29915,29936,
    29956,29976,29997,30017,30037,30057,30077,30097,30117,30136,30156,30176,
    30195,30215,30234,30253,30273,30292,30311,30330,30349,30368,30387,30406,
    30424,30443,30462,30480,30498,30517,30535,30553,30571,30589,30607,30625,
    30643,30661,30679,30696,30714,30731,30749,30766,30783,30800,30818,30835,
    30852,30868,30885,30902,30919,30935,30952,30968,30985,31001,31017,31033,
    31050,31066,31082,31097,31113,31129,31145,31160,31176,31191,31206,31222,
    31237,31252,31267,31282,31297,31312,31327,31341,31356,31371,31385,31400,
    31414,31428,31442,31456,31470,31484,31498,31512,31526,31539,31553,31567,
    31580,31593,31607,31620,31633,31646,31659,31672,31685,31698,31710,31723,
    31736,31748,31760,31773,31785,31797,31809,31821,31833,31845,31857,31869,
    31880,31892,31903,31915,31926,31937,31949,31960,31971,31982,31993,32004,
    32014,32025,32036,32046,32057,32067,32077,32087,32098,32108,32118,32128,
    32137,32147,32157,32166,32176,32185,32195,32204,32213,32223,32232,32241,
    32250,32258,32267,32276,32285,32293,32302,32310,32318,32327,32335,32343,
    32351,32359,32367,32375,32382,32390,32397,32405,32412,32420,32427,32434,
    32441,32448,32455,32462,32469,32476,32482,32489,32495,32502,32508,32514,
    32521,32527,32533,32539,32545,32550,32556,32562,32567,32573,32578,32584,
    32589,32594,32599,32604,32609,32614,32619,32624,32628,32633,32637,32642,
    32646,32650,32655,32659,32663,32667,32671,32674,32678,32682,32685,32689,
    32692,32696,32699,32702,32705,32708,32711,32714,32717,32720,32722,32725,
    32728,32730,32732,32735,32737,32739,32741,32743,32745,32747,32748,32750,
    32752,32753,32755,32756,32757,32758,32759,32760,32761,32762,32763,32764,
    32765,32765,32766,32766,32766,32767,32767,32767,32767
} ;


// Table of function -2147483648*sin(x) for x ranging from 0 to 90 degrees with an iterval of 90/128 degrees
static const int32_t minusSin32aTab[] =
{
              0,  -26352928,  -52701887,  -79042909, -105372028, -131685278, -157978697, -184248325,
     -210490206, -236700388, -262874923, -289009871, -315101295, -341145265, -367137861, -393075166,
     -418953276, -444768294, -470516330, -496193509, -521795963, -547319836, -572761285, -598116479,
     -623381598, -648552838, -673626408, -698598533, -723465451, -748223418, -772868706, -797397602,
     -821806413, -846091463, -870249095, -894275671, -918167572, -941921200, -965532978, -988999351,
    -1012316784,-1035481766,-1058490808,-1081340445,-1104027237,-1126547765,-1148898640,-1171076495,
    -1193077991,-1214899813,-1236538675,-1257991320,-1279254516,-1300325060,-1321199781,-1341875533,
    -1362349204,-1382617710,-1402678000,-1422527051,-1442161874,-1461579514,-1480777044,-1499751576,
    -1518500250,-1537020244,-1555308768,-1573363068,-1591180426,-1608758157,-1626093616,-1643184191,
    -1660027308,-1676620432,-1692961062,-1709046739,-1724875040,-1740443581,-1755750017,-1770792044,
    -1785567396,-1800073849,-1814309216,-1828271356,-1841958164,-1855367581,-1868497586,-1881346202,
    -1893911494,-1906191570,-1918184581,-1929888720,-1941302225,-1952423377,-1963250501,-1973781967,
    -1984016189,-1993951625,-2003586779,-2012920201,-2021950484,-2030676269,-2039096241,-2047209133,
    -2055013723,-2062508835,-2069693342,-2076566160,-2083126254,-2089372638,-2095304370,-2100920556,
    -2106220352,-2111202959,-2115867626,-2120213651,-2124240380,-2127947206,-2131333572,-2134398966,
    -2137142927,-2139565043,-2141664948,-2143442326,-2144896910,-2146028480,-2146836866,-2147321946,
    -2147483648
} ;

// Table of function -2147483648*sin(x) for x ranging from 0 to 90/128 degrees with an iterval of 1/128 degrees
static const int32_t minusSin32bTab[] =
{
            0,  -205887,  -411775,  -617662,  -823550, -1029437, -1235324, -1441212,
     -1647099, -1852987, -2058874, -2264761, -2470648, -2676536, -2882423, -3088310,
     -3294197, -3500085, -3705972, -3911859, -4117746, -4323633, -4529520, -4735407,
     -4941294, -5147180, -5353067, -5558954, -5764841, -5970727, -6176614, -6382501,
     -6588387, -6794273, -7000160, -7206046, -7411932, -7617818, -7823705, -8029591,
     -8235476, -8441362, -8647248, -8853134, -9059019, -9264905, -9470790, -9676676,
     -9882561,-10088446,-10294331,-10500216,-10706101,-10911986,-11117871,-11323755,
    -11529640,-11735524,-11941409,-12147293,-12353177,-12559061,-12764945,-12970828,
    -13176712,-13382595,-13588479,-13794362,-14000245,-14206128,-14412011,-14617894,
    -14823776,-15029659,-15235541,-15441423,-15647305,-15853187,-16059069,-16264950,
    -16470832,-16676713,-16882594,-17088475,-17294356,-17500237,-17706117,-17911998,
    -18117878,-18323758,-18529638,-18735517,-18941397,-19147276,-19353155,-19559034,
    -19764913,-19970791,-20176670,-20382548,-20588426,-20794304,-21000182,-21206059,
    -21411936,-21617814,-21823690,-22029567,-22235444,-22441320,-22647196,-22853072,
    -23058947,-23264823,-23470698,-23676573,-23882448,-24088323,-24294197,-24500071,
    -24705945,-24911819,-25117692,-25323565,-25529438,-25735311,-25941183,-26147056,
    -26352928
} ;

// Table of function -2147483648*cos(x) for x ranging from 0 to 90/128 degrees with an iterval of 1/128 degrees
static const int32_t minusCos32bTab[] =
{
    -2147483648,-2147483638,-2147483609,-2147483559,-2147483490,-2147483401,-2147483293,-2147483164,
    -2147483016,-2147482849,-2147482661,-2147482454,-2147482227,-2147481980,-2147481714,-2147481427,
    -2147481121,-2147480796,-2147480450,-2147480085,-2147479700,-2147479296,-2147478871,-2147478427,
    -2147477963,-2147477480,-2147476976,-2147476453,-2147475910,-2147475348,-2147474765,-2147474163,
    -2147473542,-2147472900,-2147472239,-2147471558,-2147470857,-2147470137,-2147469396,-2147468636,
    -2147467857,-2147467057,-2147466238,-2147465399,-2147464540,-2147463662,-2147462764,-2147461846,
    -2147460908,-2147459951,-2147458974,-2147457977,-2147456961,-2147455924,-2147454868,-2147453793,
    -2147452697,-2147451582,-2147450447,-2147449292,-2147448118,-2147446923,-2147445709,-2147444476,
    -2147443222,-2147441949,-2147440656,-2147439343,-2147438011,-2147436659,-2147435287,-2147433896,
    -2147432484,-2147431053,-2147429602,-2147428132,-2147426641,-2147425131,-2147423602,-2147422052,
    -2147420483,-2147418894,-2147417285,-2147415657,-2147414008,-2147412341,-2147410653,-2147408945,
    -2147407218,-2147405471,-2147403705,-2147401918,-2147400112,-2147398286,-2147396441,-2147394575,
    -2147392690,-2147390786,-2147388861,-2147386917,-2147384953,-2147382969,-2147380965,-2147378942,
    -2147376899,-2147374837,-2147372754,-2147370652,-2147368530,-2147366388,-2147364227,-2147362046,
    -2147359845,-2147357624,-2147355384,-2147353124,-2147350844,-2147348544,-2147346225,-2147343886,
    -2147341527,-2147339149,-2147336750,-2147334332,-2147331895,-2147329437,-2147326960,-2147324463,
    -2147321946
} ;

// Table of angles for Cordic algorithm:
// Item n is equal to atan(1/pow(2,n)), in units of full-circle/(2^32)
// The sum of all items is 1191650199, i.e. about 99.882965821 degrees
static const int32_t cordicAtan[] =
{/*     theta,      // n  theta(°)         atan(theta) */
    536870912,      // 0  45.000000000000  1
    316933406,      // 1  26.565051177078  1/2
    167458907,      // 2  14.036243467927  1/4
     85004756,      // 3   7.125016348902  1/8
     42667331,      // 4   3.576334374997  1/16
     21354465,      // 5   1.789910608246  1/32
     10679838,      // 6   0.895173710211  1/64
      5340245,      // 7   0.447614170861  1/128
      2670163,      // 8   0.223810500369  1/256
      1335087,      // 9   0.111905677066  1/512
       667544,      // 10  0.055952891894  1/1024
       333772,      // 11  0.027976452617  1/2048
       166886,      // 12  0.013988227142  1/4096
        83443,      // 13  0.006994113675  1/8192
        41722,      // 14  0.003497056851  1/16384
        20861,      // 15  0.001748528427  1/32768
        10430,      // 16  0.000874264214  1/65536
         5215,      // 17  0.000437132107  1/131072
         2608,      // 18  0.000218566053  1/262144
         1304,      // 19  0.000109283027  1/524288
          652,      // 20  0.000054641513  1/1048576
          326,      // 21  0.000027320757  1/2097152
          163,      // 22  0.000013660378  1/4194304
           81,      // 23  0.000006830189  1/8388608
           41,      // 24  0.000003415095  1/16777216
           20,      // 25  0.000001707547  1/33554432
           10,      // 26  0.000000853774  1/67108864
            5,      // 27  0.000000426887  1/134217728
            3,      // 28  0.000000213443  1/268435456
            1,      // 29  0.000000106722  1/536870912
            1,      // 30  0.000000053361  1/1073741824
            0       // 31  0.000000026680  1/2147483648
} ;

// Table for the cordic gain removal:
// Item n is equal to 65536*(1 - 1/gain(n)), where gain(n) is the
// cordic gain after n iterations
static const int16_t cordicAdj[] =
{ /*  adj,          // n  1/gain           */
        0,          // 0  1.00000000000000
    19195,          // 1  0.70710678118655
    24087,          // 2  0.63245553203368
    25325,          // 3  0.61357199107790
    25635,          // 4  0.60883391251775
    25713,          // 5  0.60764825625617
    25733,          // 6  0.60735177014130
    25737,          // 7  0.60727764409353
    25739,          // 8  0.60725911229889
    25739,          // 9  0.60725447933256
    25739,          // 10 0.60725332108988
    25739,          // 11 0.60725303152913
    25739,          // 12 0.60725295913895
    25739,          // 13 0.60725294104140
    25739,          // 14 0.60725293651701
    25739,          // 15 0.60725293538591
    25739,          // 16 0.60725293510314
    25739,          // 17 0.60725293503245
    25739,          // 18 0.60725293501477
    25739,          // 19 0.60725293501035
    25739,          // 20 0.60725293500925
    25739,          // 21 0.60725293500897
    25739,          // 22 0.60725293500890
    25739,          // 23 0.60725293500889
    25739,          // 24 0.60725293500888
    25739,          // 25 0.60725293500888
    25739,          // 26 0.60725293500888
    25739,          // 27 0.60725293500888
    25739,          // 28 0.60725293500888
    25739,          // 29 0.60725293500888
    25739,          // 30 0.60725293500888
    25739,          // 31 0.60725293500888
    25739           // 32 0.60725293500888
} ;


// Exported functions --------------------------------------------------------------------------------------------------

/*******************************************************************************************************************//**
 * @brief   Convertion of an unsigned integer to an ASCII string representing the decimal value of the integer
 * @param   szBuf   Pointer to the destination memory buffer. The buffer must have a minimum size of 11 bytes. At the
 *                  end of the function, the buffer[10] location will contain the string terminator ('\0'). The
 *                  buffer[9] will contain the units, the buffer[8] the tens digit and so on up to the most significant
 *                  digit of the number. The function zero-fills the buffer at the left of the most significant digit,
 *                  but returns a pointer to the first non-zero digit, if present, or to the units.
 *          uNbr    Number to be converted (0..4294967296)
 * @retval  char *  Pointer to the most significant non-zero digit of the ASCII string or to the units.
 */
char *ultoa(char *szBuf, uint32_t uNbr)
{
    if (szBuf)
    {
        int i ;
        for (i = 0 ; i < 10 ; i++) *(szBuf++) = '0' ;
        *szBuf = '\0' ;
        do
        {
            *(--szBuf) = (char)(uNbr % 10) + '0' ;
            uNbr /= 10 ;
        } while (uNbr) ;
    }
    return szBuf ;
}


/*******************************************************************************************************************//**
 * @brief   Convertion of an unsigned long integer to an ASCII string representing the decimal value of the integer
 * @param   szBuf   Pointer to the destination memory buffer. The buffer must have a minimum size of 21 bytes. At the
 *                  end of the function, the buffer[20] location will contain the string terminator ('\0'). The
 *                  buffer[19] will contain the units, the buffer[18] the tens digit and so on up to the most
 *                  significant digit. The function zero-fills the buffer at the left of the most significant digit,
 *                  but returns a pointer to the first non-zero digit, if present, or to the units.
 *          uNbr    Number to be converted (0..18446744073709551615)
 * @retval  char *  Pointer to the most significant non-zero digit of the ASCII string or to the units.
 */
char *ulltoa(char *szBuf, uint64_t uNbr)
{
    if (szBuf)
    {
        unsigned i ;
        for (i = 0 ; i < 20 ; i++) *(szBuf++) = '0' ;
        *szBuf = '\0' ;
        do
        {
            *(--szBuf) = (char)(uNbr % 10LL) + '0' ;
            uNbr /= 10LL ;
        } while (uNbr) ;
    }
    return szBuf ;
}


/*******************************************************************************************************************//**
 * @brief   Convertion of an unsigned integer to an ASCII string representing the hexadecimal value of the integer
 * @param   szBuf   Pointer to the destination memory buffer. The buffer must have a minimum size of 9 bytes. At the
 *                  end of the function, the buffer[8] location will contain the string terminator ('\0'). The
 *                  buffer[7] will contain the least significant hex digit, the buffer[6] the next digit and so on up to
 *                  the most significant hex digit of the number. The function zero-fills the buffer at the left of the
 *                  most significant digit, but returns a pointer to the first non-zero digit, if present, or to the
 *                  least significant digit.
 *          uNbr    Number to be converted (0..0xFFFFFFFF)
 * @retval  char *  Pointer to the most significant non-zero hex digit of the ASCII string or to the least significant
 *                  digit
 */
char *ultox(char *szBuf, uint32_t uNbr)
{
    static const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                               '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'} ;
    if (szBuf)
    {
        int i ;
        for (i = 0 ; i < 8 ; i++) *(szBuf++) = '0' ;
        *szBuf = '\0' ;
        do
        {
            *(--szBuf) = hex[uNbr & 0x0F] ;
            uNbr >>= 4 ;
        } while (uNbr) ;
    }
    return szBuf ;
}


/*******************************************************************************************************************//**
 * @brief   Convertion of a signed integer to an ASCII string representing the decimal value of the integer
 * @param   szBuf   Pointer to the destination memory buffer. The buffer must have a minimum size of 12 bytes. At the
 *                  end of the function, the szBuf[11] location will contain the string terminator ('\0'). The
 *                  szBuf[10] will contain the units, the szBuf[8] the tens digit and so on up to the most significant
 *                  digit of the number in szBuf[1]. The sign character is in szBuf[0]. The function zero-fills the
 *                  buffer at the left of the most significant digit, but returns a pointer to the first non-zero digit,
 *                  if present, or to the units.
 *          iNbr    Number to be converted (-2147483648..2147483647)
 * @retval  char *  Pointer to the most significant non-zero digit of the ASCII string or to the units.
 */
char *sltoa(char *szBuf, int32_t iNbr)
{
    if (szBuf)
    {
        // Aggiusta il segno
        if (iNbr >= 0)
        {
            *(szBuf++) = '+' ;
        }
        else
        {
            *(szBuf++) = '-' ;
            iNbr = -iNbr ;
        }
        // Converte e restituisce il numero
        return ultoa(szBuf, (uint32_t)iNbr) ;
    }
    return szBuf ;
}


/*******************************************************************************************************************//**
 * @brief   Convertion of a signed long integer to an ASCII string representing the decimal value of the integer
 * @param   szBuf   Pointer to the destination memory buffer. The buffer must have a minimum size of 21 bytes. After the
 *                  termination of the function, the buffer[20] location will contain the string terminator ('\0'). The
 *                  buffer[19] will contain the units, the buffer[18] the tens digit and so on up to the most
 *                  significant digit in szBuf[1]. The sign character is in szBuf[0]. The function zero-fills the
 *                  buffer at the left of the most significant digit, but returns a pointer to the first non-zero digit,
 *                  if present, or to the units.
 *          uNbr    Number to be converted (-9223372036854775808..9223372036854775807)
 * @retval  char *  Pointer to the most significant non-zero digit of the ASCII string or to the units.
 */
char *slltoa(char *szBuf, int64_t iNbr)
{
    char *str ;
    if (NULL != (str = szBuf))
    {
        // Aggiusta il segno
        if (iNbr >= 0LL)
        {
            // Converte
            str = ulltoa(szBuf, (uint64_t)iNbr) ;
            szBuf[0] = '+' ;
        }
        else
        {
            str = ulltoa(szBuf, (uint64_t)(-iNbr)) ;
            szBuf[0] = '-' ;
        }
    }
    return str ;
}

/*******************************************************************************************************************//**
 * @brief Calculates the square root of a 32 bit unsigned integer. The result is such that the following holds:
 *          Radicand = Root^2 + Remainder
 * @param Rad   Radicand value. Any uint32_t value is valid
 * @param pRem  Pointer to the uint32_t variable that will store the Remainder at the end of the function. If the
 *              pointer value is NULL, the remainder is discarded.
 * @return      The Root value
 */
uint16_t SqrtU32(uint32_t Rad, uint32_t *pRem)
{
    unsigned i ;
    uint32_t Rem, Tmp ;
    uint16_t Roo ;
    for (i = 0, Rem = 0, Roo = 0 ; i < 16 ; i++)
    {
        Rem = (Rem << 2) | (Rad >> 30) ;
        Rad <<= 2 ;
        Tmp = (Roo << 2) | 0x01 ;
        Roo <<= 1 ;
        if (Rem >= Tmp) { Rem -= Tmp ; Roo |= 1 ; }
    }
    if (pRem) *pRem = Rem ;
    return Roo ;
}


/*******************************************************************************************************************//**
 * @brief  Floating point division and remainder
 * @param  *fQuot  Pointer to the floating pointer dividend. At the end the functions stores the quotient of the
 *                 division
 * @param  fDiv    Divisor
 * @retval         Remainder of the division. The sign is equal to the sign of the quotient
 */
double frem( double *fQuot, double fDiv )
{
    double fTmp ;
    fTmp = *fQuot ;
    return fTmp - (fDiv * (*fQuot = trunc(fTmp / fDiv))) ;
}


/******************************************************************************************************************//**
 * @brief  Convert a floating point value to an ASCII string
 * @param  szBuf    Pointer to a buffer where the converted string will be placed. The size of the buffer must be (at
 *                  least) 35 bytes. At the end of the function szBuf[0] will contain the sign, szBuf[1] will contain
 *                  the most significand digit of the mantissa, and so on.
 * @param  fNum     Floating point variable to be converted
 * @param  iNFrac   Number of fractional digits to print. The value 0 forces the print of the decimal point with no
 *                  fractional digits. The value -1 suppresses the print of the decimal point.
 * @retval char *   Pointer to the converted string. The first character is the sign ('+' or '-')
 */
char *ftoa(char *szBuf, double fNum, int iNFrac)
{
    static const double _10[] =
    {1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,1e11,1e12,1e13,1e14,1e15,1e16 } ;

    // Checks the argument
    if (szBuf != NULL)
    {
        char cSign ;
        // Go to the end of the string
        szBuf = &(szBuf[35]) ;
        *(--szBuf) = '\0' ;
        // Check the sign of the argument
        if (fNum < 0.0)
        {
            cSign = '-' ;
            fNum = -fNum ;
        }
        else cSign = '+' ;
        // Check the maximum printable number
        if (fNum < 1e17)
        {
            // Must print the fractional part
            if (iNFrac >= 0)
            {
                // Not more than 16 digits
                if (iNFrac > 16) iNFrac = 16 ;
                fNum = round(fNum * _10[iNFrac]) ;
                // Prints each digit
                for ( ; iNFrac > 0 ; --iNFrac )
                {
                    *(--szBuf) = '0' + (uint8_t)frem(&fNum, 10.0) ;
                }
                *(--szBuf) = '.' ;
            }
            // Must print only the integer part
            else fNum = round(fNum) ;
            // Prints the mantissa
            do
            {
                *(--szBuf) = '0' + (uint8_t)frem(&fNum, 10.0) ;
            } while (fNum >= 1.0) ;
            *(--szBuf) = cSign ;
        }
    }
    // Returns the pointer
    return szBuf ;
}


/*******************************************************************************************************************//**
 * @brief   String edit utility through a generic communication device
 * @param   *pfTx       Pointer to the transmit function used to echo the received characters. If NULL the echo is
 *                      suppressed. The function must be in the following form:
 *                          void (*pfTx)(char ch)
 *                      where ch is the character to be printed
 * @param   *pfRx       Pointer to the receive function. It cannot be NULL. The function must be in the following form:
 *                          char (*pfRx)(void)
 *                      where ch is the received character
 * @param   *str        Pointer to the destination buffer of the string. The value NULL is not accepted
 * @param   size        Size of the destination buffer. It must be greater than or equal to 1. The destination buffer
 *                      should store the string and the termination character, so this argument must be set to one
 *                      character more than the maximum string length that the function should handle. If size is
 *                      exactly equal to 1 then the function does not accept characters other than 'new line' and
 *                      generates an empty string
 * @retval  char *      Value of the argument 'str' or NULL in case of errors
 */
char *EditString(void (*pfTx)(char), char (*pfRx)(void), char *str, size_t size)
{
    char ch ;
    uint32_t curs = 0 ;
    // Check the arguments
    if ((NULL == pfRx) || (NULL == str) || (0 == (size--))) return NULL ;
    // Edit loop
    while (1)
    {
        // Wait for a new char
        ch = pfRx() ;
        // Is it printable?
        if (isprint(ch))
        {
            // Is there room in the string?
            if (curs < size)
            {
                // Echoes the received character
                if (NULL != pfTx) (*pfTx)(ch) ;
                // Stores the received character
                str[curs++] = ch ;
            }
        }
        // Is it "backspace" or DEL?
        else if (('\b' == ch) || ('\177' == ch))
        {
            // At least one char to delete
            if (curs > 0)
            {
                // Echo required
                if (NULL != pfTx)
                {
                    // Echoes the backspace
                    (*pfTx)('\b') ;
                    (*pfTx)(' ') ;
                    (*pfTx)('\b') ;
                }
                // Decrements the cursor
                --curs ;
            }
        }
        // Is it "carriage return"?
        else if ('\r' == ch) break ;
    }
    // String termination
    str[curs] = '\0' ;
    return str ;
}


/*******************************************************************************************************************//**
 * @brief   Search of the first non-BLANK character in a string. The non-BLANK characters are the ones that return FALSE
 *          in the stantdard isspace() function.
 * @param   pCur:   Pointer to the current string pointer. The function starts scanning the string from the position
 *                  indicated by the pointer at the beginning, then it leaves the pointer at the first non-blank
 *                  character or at the string terminator.
 * @retval  First non-blank character or '\0' if the string terminates before finding any non-blank character
 */
char skipblank(const char **pCur)
{
    const char *str ;
    char ch  = '\0' ;
    // Checks the pointers
    if ( (pCur != NULL) && ((str = *pCur) != NULL) )
    {
        // Skips the white spages
        while (isspace(ch = *str)) ++str ;
        // Updates the cursor
        *pCur = str ;
    }
    // First not blank char
    return ch ;
}


/*******************************************************************************************************************//**
 * @brief   Compares the first non-blank character in a string with a sample character.
 * @param   ch:     Sample character to be compared. The comparision is case-sensitive
 * @param   pCur:   Pointer to the current string pointer. The function search the first non blank character in the
 *                  string, then it compares it with the sample character. If the characters are equal then the string
 *                  updates the pointer the first character after the searched one.
 * @retval  - true: The characters are equal. The pointer is updated
 *          - false: The characters are different. The pointer is unchanged
 */
bool charcmp(char ch, const char **pCur)
{
    if (skipblank(pCur) == ch)
    {
        ++(*pCur) ;
        return true ;
    }
    return false ;
}


/*******************************************************************************************************************//**
 * @brief   Compares a token in a string with a sample token. The comparision is case-insensitive
 * @param   tok:    Pointer to the sample token. It must be a LOWERCASE NOT-BLANK string.
 * @param   pCur:   Pointer to the current string pointer. The function search the first token in the string, then it
 *                  compares it with the sample token. If the tokens are equal then the string updates the pointer to
 *                  the first character after the token.
 * @retval  - true: the token are identical. The pointer has been updated
 *          - false: the tokens are different. The pointer is unchanged
 */
bool tokencmp(const char *tok, const char **pCur)
{
    const char *ptr ;
    char ch1, ch2 ;
    ch1 = *tok ;
    ch2 = skipblank(pCur) ;
    ptr = *pCur ;
    while (ch1 != '\0')
    {
        if (tolower(ch1) != tolower(ch2)) return false ;
        ch1 = *(++tok) ;
        ch2 = *(++ptr) ;
    }
    if (isalnum(ch2)) return false ;
    *pCur = ptr ;
    return true ;
}


/*******************************************************************************************************************//**
 * @brief   Converts an ASCII string, representing an unsigned decimal number, to a 32 bit unsigned integer. The
 *          function skips all the non-blank characters before the number, then converts all the consecutive numerical
 *          characters. The function leaves the pointer to the first non-numerical character after the converted number.
 *          The functions signals an error if it does not find any numerical character
 * @param   pCur:   Pointer to the current string pointer
 * @param   pNbr:   Pointer to the unsigned 32 bit variable used to store the converted number
 * @retval  - true:     Conversion competed without errors
 *          - false:    Errors detected.
 */
bool atoul(const char **pCur, uint32_t *pNbr)
{
    const char *str ;
    // Checks the pointers
    if ( pCur && ((str = *pCur) != NULL) )
    {
        uint8_t ch ;
        uint32_t ul ;
        // Skips the blank spaces
        ch = skipblank( &str ) ;
        // At least one digit
        if (_todec(ch))
        {
            // Converts the first digit
            ul = (uint32_t)ch ; ch = *(++str) ;
            // Converts all the other digits
            for ( ; _todec(ch) ; ch = *(++str))
            {
                // Check and convertion of a new digit
                if (ul > 429496729u) goto __error ;
                ul *= 10u ;
                ul += (uint32_t)ch ;
                if (ul < (uint32_t)ch) goto __error ;
            }
            // Update pointer and result
            *pCur = str ; if (pNbr) *pNbr = ul ;
            // All done
            return true ;
        }
    }
    // Errors detected
    __error: return false ;
}


/*******************************************************************************************************************//**
 * @brief   Converts an ASCII string, representing a signed decimal number, to a 32 bit signed integer. The function
 *          skips all the non-blank characters before the number, then converts all the consecutive numerical
 *          characters. The function leaves the pointer to the first non-numerical character after the converted number.
 *          The functions signals an error if it does not find any numerical character
 * @param   pCur:   Pointer to the current string pointer
 * @param   pNbr:   Pointer to the unsigned 32 bit variable used to store the converted number
 * @retval  - true:     Conversion competed without errors
 *          - false:    Errors detected.
  */
bool atosl(const char **pCur, int32_t *pNbr)
{
    const char *str ;
    // Checks the pointers
    if ( pCur && ((str = *pCur) != NULL) )
    {
        uint8_t ch ;
        uint8_t sign ;
        uint32_t ul ;
        // Skips the blank spaces
        ch = skipblank( &str ) ;
        // Detect the sign of the number
        if ((ch == '+') || (ch == '-'))
        {
            sign = ch ;
            ch = *(++str) ;
        }
        // If sign is absent, assumes the number is positive
        else sign = '+' ;
        // At least one digit
        if (_todec(ch))
        {
            // Converts the first digit
            ul = (uint32_t)ch ; ch = *(++str) ;
            // Converts all the other digits
            for ( ; _todec(ch) ; ch = *(++str))
            {
                // Check and convert a new digit
                if (ul > 214748364u) goto __error ;
                ul *= 10u ;
                ul += (uint32_t)ch ;
                if (ul < (uint32_t)ch) goto __error ;
            }
            // Should be negative
            if (sign == '-')
            {
                // Check the most negative number
                if (ul > 0x80000000) goto __error ;
                // Adjust sign
                ul = ~ul + 1 ;
            }
            // Check the most positive number
            else if (ul >= 0x80000000) goto __error ;
            // Update pointer and number
            *pCur = str ; if (pNbr) *pNbr = (int32_t)ul ;
            // Done
            return true ;
        }
    }
    // Errore
    __error: return false ;
}


/*******************************************************************************************************************//**
 * @brief   Converts an ASCII string, representing an unsigned hexadecimal number, to a 32 bit unsigned integer. The
 *          function skips all the non-blank characters before the number, then converts all the consecutive hex
 *          characters. The function accepts the following characters: '0'..'9', 'A'..'F', 'a'..'f'. The function leaves
 *          the pointer to the first non-hex character after the converted number. The function signals an error if it
 *          does not find any hex character.
 * @param   pCur:   Pointer to the current string pointer
 * @param   pNbr:   Pointer to the unsigned 32 bit variable used to store the converted number
 * @retval  - true:     Conversion competed without errors
 *          - false:    Errors detected.
  */
bool xtoul(const char **pCur, uint32_t *pNbr)
{
    const char *str ;
    // Verifica i puntatori
    if ( (pCur != NULL) && ((str = *pCur) != NULL) )
    {
        uint8_t ch ;
        uint32_t ul ;
        // Salta gli spazi iniziali
        ch = skipblank( &str ) ;
        // Ci vuole almeno un digit
        if ( _tohex(ch) )
        {
            // Converte il primo digit
            ul = (uint32_t)ch ; ch = *(++str) ;
            // Conversione dei digit successivi
            for ( ; _tohex(ch) ; ch = *(++str) )
            {
                // Primo check dell'overflow
                if (ul > 0x0FFFFFFF) goto __error ;
                // Check e conversione del digit
                ul <<= 4 ;
                ul += (uint32_t)ch ;
                // Secondo check dell'overflow
                if (ul < (uint32_t)ch) goto __error ;
            }
            // Update pointer and result
            *pCur = str ; if (pNbr) *pNbr = ul ;
            // Done
            return true ;
        }
    }
    // Errors
    __error: return false ;
}


/*******************************************************************************************************************//**
 * @brief   Cordic algorithm in vectoring mode
 * @param   pT          Pointer to the starting vector angle in (full-turn)/pow(2,32) units. At the end the angle should
 *                      be the sum of the starting angle and the angle of the starting vector (atan2(x, y))
 * @param   pX          Pointer to the starting vector X coordinate. At the end it contains the magnitude of the vector
 *                      multiplied by a constant system gain (see below)
 * @param   pY          Pointer to the starting vector Y coordinate. At the end it should be a null value
 * @return  unsigned    Number of iterations executed
 *
 * @note    The Cordic algorithm applies a certain gain after each iteration. The gain is given by:
 *              k[i] = 1/cos(atan(1/pow(2,i)))
 *          hence, at the end of of the nth iteration, the overall gain is:
 *              gain = k[1] * k[2] * ... * k[n]
 *          For a large number of iteration, the gain tends to the following limit:
 *              gain = 1.64676025812107 (for n large)
 *          This is defined in the costant CORDIC_GAIN. The constant is accurate up to 16 bits after 8 iteration, or
 *          up to 32 bits after 17 iterations.
 *
 * @note    This function lasts about 1.25us/loop running on a 32MHz Cortex-M3 core. On average it takes about 16 loops
 *          in most cases. The worst case is 32 loops.
 */
unsigned cordicVectoring(int32_t *pT, int32_t *pX, int32_t *pY)
{
    unsigned i ;
    const int32_t *pA ;
    int32_t t, x, y ;
    // Starting values
    t = *pT ; x = *pX ; y = *pY ;
    // Loop for nulling y
    for (i = 0, pA=cordicAtan ; (0 != y) && (lengthof(cordicAtan) > i) ; i++, pA++)
    {
        register int32_t dt, dx, dy ;
        // Corrections for this iteration
        dt = *pA ;
        dx = RSHIFT(y, i) ;
        dy = RSHIFT(x, i) ;
        // Rotate CCW (the sign y is equal to the sign of dy)
        if (0 == (0x80000000 & (y ^ dy))) { t += dt ; x += dx ; y -= dy ; }
        // Rotate CW (the sign y is not equal to the sign of dy)
        else { t -= dt ; x -= dx ; y += dy ; }
    }
    // Resulting values
    *pT = t ; *pX = x ; *pY = y ;
    // Number of iterations
    return i ;
}


/*******************************************************************************************************************//**
 * @brief   Cordic algorithm in rotating mode
 * @param   pT          Pointer to the starting vector angle in (full-turn)/pow(2,32) units. t the end the angle should
 *                      be zero
 * @param   pX          Pointer to the starting vector X coordinate. At the end it contains the following value:
 *                      K*(x*cos(theta) - y*sin(theta))
 *                      where k is the algorithm gain (see below)
 * @param   pY          Pointer to the starting vector Y coordinate. At the end it contains the following value:
 *                      K*(x*sin(theta) + y*cos(theta))
 *                      where k is the algorithm gain (see below)
 * @return  unsigned    Number of iterations executed
 *
 * @note    The Cordic algorithm applies a certain gain after each iteration. The gain is given by:
 *              k[i] = 1/cos(atan(1/pow(2,i)))
 *          hence, at the end of of the nth iteration, the overall gain is:
 *              gain = k[1] * k[2] * ... * k[n]
 *          For a large number of iteration, the gain tends to the following limit:
 *              gain = 1.64676025812107 (for n large)
 *          This is defined in the costant CORDIC_GAIN. The constant is accurate up to 16 bits after 8 iteration, or
 *          up to 32 bits after 17 iterations.
 */
unsigned cordicRotating(int32_t *pT, int32_t *pX, int32_t *pY)
{
    unsigned i ;
    const int32_t *pA ;
    int32_t t, x, y ;
    // Starting values
    t = *pT ; x = *pX ; y = *pY ;
    // Loop for nulling t
    for (i = 0, pA = cordicAtan ; (0 != t) && (lengthof(cordicAtan) > i) ; i++, pA++)
    {
        register int32_t dt, dx, dy ;
        // Corrections for this iteration
        dt = *pA ;
        dx = RSHIFT(y, i) ;
        dy = RSHIFT(x, i) ;
        // Rotate CCW
        if (0 > t) { t += dt ; x += dx ; y -= dy ; }
        // Rotate CW
        else       { t -= dt ; x -= dx ; y += dy ; }
    }
    // Resulting values
    *pT = t ; *pX = x ; *pY = y ;
    // Number of iterations
    return i ;
}


/*******************************************************************************************************************//**
 * @brief Returns the angle whose tangent is y/x. The angle is espressed in (full circle)/65536
 * @param   y   Numerator of the y/x ratio. Can be any value
 * @param   x   Denominator of the y/x ratio. Can be any value
 * @return  Angle whose tangent is the ratio y/x. The angle always has the same sign of y, and it is in the range
 *          -32768..32767
 *
 * @note    This function takes about 11.1us to execute on an STM32L151 Cortex M3 core 32MHz
 */
int32_t tAtan2(int32_t y, int32_t x)
{
    int32_t a ;
    a = 0 ;
    cordicVectoring(&a, &x, &y) ;
    a = RSHIFT(a, 16u) ;
    return (0 <= x)? a : ((0 <= a)? (a - 32768L) : (a + 32768L)) ;
}


/*******************************************************************************************************************//**
 * @brief   Cartesian to polar coordinate conversion
 * @param   x   Cartesian x-coordinate
 * @param   y   Cartesian y-coordinate
 * @return  Structure tPolar_t holding the following elements:
 *          radius  distance from origin to the point (x,y): sqrt(x*x + y*y)
 *          theta   angle relative to the x-axis espressed in (full_circle)/65536
 */
tPolar_t tPolar(int32_t x, int32_t y)
{
    register tPolar_t p ;
    int32_t a ;
    unsigned i ;
    if (0 == y)
    {
        if (0 <= x) { p.radius = x ;  p.theta  = 0L ; }
        else        { p.radius = -x ; p.theta  = -32768L ; }
    }
    else if (0 == x)
    {
        if (0 <= y) { p.radius = y ;  p.theta  = 16384L ; }
        else        { p.radius = -y ; p.theta  = -16384L ; }
    }
    else
    {
        a = 0 ;
        i = cordicVectoring(&a, &x, &y) ;
        a = RSHIFT(a, 16u) ;
        x = MULRSH(x, 65536L - cordicAdj[i], 16u) ;
        if (0 > x) { x = -x ; a += 32768L ; }
        if (32767L < x) x =  32767L ;
        p.radius = x ;
        p.theta  = a ;
    }
    return p ;
}


/*******************************************************************************************************************//**
 * @brief   Polar to Cartesian coordinate conversion
 * @param   radius  distance from origin to the point (x,y)
 * @param   theta   angle relative to the x-axis espressed in (full_circle)/65536
 * @return  Structure tCartesian_t holding the following elements:
 *          x   Cartesian x-coordinate
 *          y   Cartesian y-coordinate
 */
tCartesian_t tCartesian(int16_t radius, int16_t theta)
{
    register tCartesian_t p ;
    int32_t x, y, a ;
    unsigned i ;
    if (0 != radius)
    {
        if ((16384L <= theta) || (-16384 > theta))
        {
            theta += -32768L ;
            radius = -radius ;
        }
        x = radius ;
        y = 0 ;
        a = theta << 16u ;
        i = cordicRotating(&a, &x, &y) ;
        p.x = MULRSH(x, 65536L - cordicAdj[i], 16u) ; ;
        p.y = MULRSH(y, 65536L - cordicAdj[i], 16u) ; ;
    }
    else
    {
        p.x = 0 ;
        p.y = 0 ;
    }
    return p ;
}


/*******************************************************************************************************************//**
 * @brief  Fast function sin() calculation
 * @param  Angle in unit of round-angle/65536
 * @retval Value of sin(x) multiplied by 32767
 *
 * The input angle is expressed in units of 65536-th of the round angle, that is 0,0054931640625°, or 19,775390625", or
 * about 9.5874e-5 rad. The function reduces the angle to the first quadrant, then translates the 10 most significant
 * bits, using a 1024 entries table. The remaining least significant 4 bits are linearly interpolated. The sign of the
 * result is adjusted to the original quadrant and multiplied by 32767
 */
int16_t tSin(int16_t alpha)
{
    int16_t a, s, d ;
    a = alpha & 0x3FFF ;
    if (alpha & 0x4000) a = 0x3FFF - a ;
    s = sinTab[(a>>4)] ; d = sinTab[(a>>4) + 1] - s ;
    s += ((d*(a & 0x000F)+0x0008) >> 4) ;
    return (alpha >= 0)? s : -s ;
}


/*******************************************************************************************************************//**
 * @brief  Fast function sin() calculation
 * @param  Angle in unit of round-angle/65536
 * @retval Value of sin(x) multiplied by pow(2,30) = 1073741824
 *
 * The input angle is expressed in units of 65536-th of the round angle, that is 0,0054931640625 degees, or about
 * 19,775390625 arc-seconds, or about 9,5873799242852576857380474343247e-5 rad (i.e. PI/pow(2,15)).
 * The accuracy of the calculated sin() value is better than 0.005%
 */
int32_t sin32( int16_t alpha )
{
    uint16_t a, b ;
    int64_t s ;

    /* Transalte to quadrant I and II: [-32768, -1] --> [0, 32767] */
    a = (alpha >= 0)? ((uint16_t)alpha) : ((uint16_t)alpha + 32768u) ;
    /* Fold-back to quadrant I: [16385,32767] --> [16383,1]  */
    if (a > 16384u) a = 32768u - a ;
    /* Divide in 2 angles for indexing two 128 entries tables  */
    b = a & 0x7Fu ; // Small angle
    a = a >> 7u ;   // Large angle
    /* Use identity: sin(a+b) = sin(a)*cos(b) + cos(a)*sin(b) */
    s =   (int64_t)minusSin32aTab[a]      * (int64_t)minusCos32bTab[b]
        + (int64_t)minusSin32aTab[128u-a] * (int64_t)minusSin32bTab[b] ;
    /* Round to nearest */
    s = (s + (1LL<<31u)) >> 32u ;
    /* Adjust for negative angles */
    return (alpha < 0L)? -s : s ;
}


/*******************************************************************************************************************//**
 * @brief  Tiny printf-type string parser.
 * @param  *fp      Pointer to the function to be used to print the character. The function must be in the following
 *                  form:
 *                      void (*cfp)(char ch)
 *                  where ch is the character to be printed
 * @param  *fmt     Format string. The format is like the standard printf format, with the following limitations:
 *                  - Only the %d, %u, %x, %X, %f, %s, %S formats are accepted. The h and l flags are not accepted
 * @param  ...      Variable arguments
 * @retval int      Numbers of arguments accepted and printed
 */
enum
{
	_dubl_ = 0x10,
	_sign_ = 0x08,
	_left_ = 0x04,
	_prec_ = 0x02,
	_zero_ = 0x01
} ;
static void __print( void (*fp)(char), char *str, int width, int prec, int flags, int size )
{
    char ch ;
	// Pad with leading spaces
	if (!(flags & _left_)) {while (width > size++) (*fp)(' ');}
	// Print the number
	while ((ch = *(str++)) != '\0') (*fp)(ch) ;
	// Pad with trailing spaces
	if (flags & _left_) {while (width > size++) (*fp)(' ');}
}
int tiny_vprintf( void (*fp)(char), const char *fmt, va_list ap)
{
    char buf[35] ;
    char ch ;
    char *str ;
    int width, prec, flags, size ;
    int acnt ;
    // Scans the format string
    for (acnt = 0 ; (ch = *fmt) != '\0' ; fmt++)
    {
        // Detects a format field
        if (ch == '%')
        {
            // Clears all the field options
            flags = prec = width = 0 ;
            // Parse the flags
            while (1)
            {
                // Get the new char
                ch = *(++fmt) ;
                // Forces the print of the sign
                if (ch == '+') flags |= _sign_ ;
                // Forces the left-justification
                else if (ch == '-') flags |= _left_ ;
                else break ;
            }
            // Starts the 'width' field
            if (isdigit(ch))
            {
                // Forces the print of leading zeros
                if (ch == '0') flags |= _zero_ ;
                do
                {
                    // Get the 'width' field
                    width = 10u*width + (ch -'0') ;
                } while (isdigit(ch = *(++fmt))) ;
            }
            // Starts the 'prec' field
            if (ch == '.')
            {
                flags |= _prec_ ;
                // Get the prec field
                while (isdigit(ch = *(++fmt))) prec = 10u*prec + (ch -'0') ;
            }
            // Get the 'long' prefix
            if ((ch == 'l')||(ch == 'L'))
            {
                flags |= _dubl_ ;
                ch = *(++fmt) ;
            }
            // Decode the type field
            switch (ch)
            {
                case 'c':
                case 'C':
                    // Prints the character
                    (*fp)((uint32_t)va_arg(ap, uint32_t)) ;
                    ++acnt ;
                    break ;
                case 'd':
                case 'i':
                    {
                        if (flags&_dubl_)
                        {
                            int64_t ld ;
                            // Get the argument
                            ld = va_arg(ap, int64_t) ;
                            // Convert to string
                            str = slltoa(buf, ld) ;
                            // Number of digits
                            size = &(buf[21]) - str ;
                            // Adjust the precision
                            if (!(flags&_prec_) && (flags&_zero_)) prec = width ;
                            if (prec > 20) prec = 20 ;
                            if (size < prec) size = prec ;
                            str = &(buf[21-size]) ;
                        }
                        else
                        {
                            int32_t d ;
                            // Get the argument
                            d = (int32_t)va_arg(ap, int32_t) ;
                            // Convert to string
                            str = sltoa(buf, d) ;
                            // Number of digits
                            size = &(buf[11]) - str ;
                            // Adjust the precision
                            if (!(flags&_prec_) && (flags&_zero_)) prec = width ;
                            if (prec > 10) prec = 10 ;
                            if (size < prec) size = prec ;
                            str = &(buf[11-size]) ;
                        }
                        // Add the sign if required
                        if ((buf[0] == '-') || (flags & _sign_))
                        {
                            *(--str)=buf[0] ;
                            ++size ;
                        }
                    }
                    __print(fp, str, width, prec, flags, size); ++acnt;
                    break ;
                case 'f':
                    {
                        // Adjust the default precision
                        if (!(flags & _prec_)) prec = -1 ;
                        else if (prec > 16) prec = 16 ;
                        // Get the argument and convert
                        str = ftoa(buf, (double)va_arg(ap, double), prec) ;
                        // Overflow: cannot print            12345678901234567890123456789012345
                        if (str[0]=='\0') strcpy(str = buf, "#################.################") ;
                        // Shouldn't print the screen if positive
                        if (!(flags & _sign_) && (*str == '+')) ++str ;
                        // Calculates the size of the string
                        size = &buf[sizeof(buf)-1] - str ;
                    }
                    __print(fp, str, width, prec, flags, size); ++acnt;
                    break ;
                case 's':
                    {
                        // Get the argument
                        str = (char *)va_arg(ap, char *) ;
                        // Calculate the size of the string
                        size = strlen(str) ;
                    }
                    __print(fp, str, width, prec, flags, size); ++acnt;
                    break ;

#ifdef WCHAR_ENABLE
                case 'S':
                    {
                        WCHAR *sp ;
                        WCHAR sc ;
                        unsigned size ;
                        // Get the argument
                        sp = (WCHAR *)va_arg(ap, WCHAR *) ;
                        // Calculate the size of the string
                        size = wsize(sp) ;
                        // Pad with leading spaces
                        if (!(flags & _left_))
                        {
                            while (width > size++) (*fp)(' ') ;
                        }
                        // Print the string up to the terminator
                        while ((sc = *(sp++)) != 0) (*fp)((uint32_t)sc) ;
                        // Pad with trailing spaces
                        if (flags & _left_)
                        {
                            while (width > size++) (*fp)(' ') ;
                        }
                        ++acnt ;
                    }
                    break ;
#endif

                case 'u':
                    {
                        if (flags&_dubl_)
                        {
                            int64_t lu ;
                            // Get the argument
                            lu = va_arg(ap, uint64_t) ;
                            // Convert to string
                            str = ulltoa(buf, lu) ;
                            // Number of digits
                            size = &(buf[21]) - str ;
                            // Adjust the precision
                            if (!(flags&_prec_) && (flags&_zero_)) prec = width ;
                            if (prec > 21) prec = 21 ;
                            if (size < prec) size = prec ;
                            str = &(buf[21-size]) ;
                        }
                        else
                        {
                            uint32_t u ;
                            // Get the argument
                            u = va_arg(ap, uint32_t) ;
                            // Convert to string
                            str = ultoa(buf, u) ;
                            // Number of digits
                            size = &(buf[10]) - str ;
                            // Adjust the precision
                            if (!(flags&_prec_) && (flags&_zero_)) prec = width ;
                            if (prec > 10) prec = 10 ;
                            if (size < prec) size = prec ;
                            str = &(buf[10-size]) ;
                        }
                    }
                    __print(fp, str, width, prec, flags, size); ++acnt;
                    break ;
                case 'x':
                case 'X':
                    {
                        uint32_t x ;
                        // Get the argument
                        x = (uint32_t)va_arg(ap, uint32_t) ;
                        // Convert to string
                        str = ultox(buf, x) ;
                        // Number of digits
                        size = &(buf[8]) - str ;
                        // Adjust the precision
                        if (!(flags&_prec_) && (flags&_zero_)) prec = width ;
                        if (prec > 8) prec = 8 ;
                        if (size < prec) size = prec ;
                        str = &(buf[8-size]) ;
                    }
                    __print(fp, str, width, prec, flags, size); ++acnt;
                    break ;
                default:
                    // Unknown field
                    (*fp)(ch) ;
                    break ;
            }
        }
        // Not a format field
        else (*fp)(ch) ;
    }
    return acnt ;
}

#ifdef WCHAR_ENABLE
/*******************************************************************************************************************//**
 * @brief   Returns the lenght of a wide-char string
 * @param   *pSrc       Pointer to the wide-char string source
 * @retval  unsigned    String lenght (the terminating NUL character is not included)
 */
unsigned wsize(const WCHAR *pSrc)
{
    unsigned i = 0 ;
    if (pSrc != NULL) for ( ; *(pSrc++) != 0 ; i++) ;
    return i ;
}
#endif

#ifdef WCHAR_ENABLE
/*******************************************************************************************************************//**
 * @brief   wide-char string copy
 * @param   *pDst   Pointer to the wide-char string destination
 * @param   *pSrc   Pointer to the wide-char string source
 * @retval  *pDst   The same pointer to the wide-char string destination
 */
WCHAR *wcpy(WCHAR *pDst, const WCHAR *pSrc)
{
    if (pDst && pSrc)
    {
        WCHAR *pd = pDst ;
        while ((*(pd++) = *(pSrc++)) != 0) ;
    }
    return pDst ;
}
#endif

#ifdef WCHAR_ENABLE
/*******************************************************************************************************************//**
 * @brief   wide-char string concatenation
 * @param   *pDst   Pointer to the wide-char string destination. The buffer must be large enough to hold the resulting
 *                  concatenated string
 * @param   *pSrc   Pointer to the wide-char string source
 * @retval  *pDst   The same pointer to the wide-char string destination
 */
WCHAR *wcat(WCHAR *pDst, const WCHAR *pSrc)
{
    if (pDst)
    {
        WCHAR *pd = pDst ;
        for ( ; (*(pd)) != 0 ; pd++) ;
        wcpy(pd, pSrc) ;
    }
    return pDst ;
}
#endif

/* END OF FILE ********************************************************************************************************/
