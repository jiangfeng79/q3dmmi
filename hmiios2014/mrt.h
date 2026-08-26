#ifndef MRT_H
#define MRT_H

#include <QString>
#include <QVector>
class WPT
{
public:
    WPT()
    {
        latitude = 0;
        longitude = 0;
    }
    WPT(QString a_sName, QString a_name, float a_lat, float a_long)
    {
        sName = a_sName;
        name = a_name;
        latitude = a_lat;
        longitude = a_long;
    }
    QString sName;
    QString name;
    float latitude;
    float longitude;
};

class WPTS
{
public:
    WPTS();
    QVector<WPT> m_wpts;
};

WPTS::WPTS()
{
    m_wpts << WPT("MRTALJ", "ALJUNIED MRT", 1.31637, 103.88270) << WPT("MRTAMK", "ANG MO KIO MRT", 1.37, 103.8496);
}

static const float mrt[] = {
    103.94910, 1.37303,    // 34
    103.94480, 1.35372,    // 40
    103.95310, 1.34371,    // 35
    103.94700, 1.32749,    // 39
    103.93020, 1.32405,    // 4
    103.9129, 1.32107,     // 20
    103.9033, 1.31969,     // 16
    103.89290, 1.31809,    // 30
    103.88270, 1.31637,    // 1
    103.87180, 1.31154,    // 22
    103.86320, 1.30752,    // 25
    103.8559, 1.30006,     // 6
    103.8525, 1.29282,     // 14
    103.85210, 1.28397,    // 32
    103.84590, 1.27696,    // 38
    103.83900, 1.28168,    // 29
    103.82680, 1.28639,    // 37
    103.81680, 1.28971,    // 33
    103.80650, 1.29419,    // 31
    103.7985, 1.30245,     // 13
    103.79030, 1.30726,    // 8
    103.778682, 1.311468,  // 54
    103.7653, 1.31493,     // 12
    103.7424, 1.33237,     // 19
    103.7326, 1.34209,     // 18
    103.72100, 1.34455,    // 24
    103.7058, 1.33948,     // 7
    103.697103, 1.337630,  // 55
    103.678283, 1.327826,  // 56

    103.7424, 1.33237,     // 19
    103.7495, 1.34924,     // 10
    103.7518, 1.35955,     // 17
    103.7443, 1.38505,     // 11
    103.747344, 1.397551,  // 48
    103.76180, 1.42555,    // 21
    103.774102, 1.432547,  // 47
    103.786444, 1.437049,  // 46
    103.800995, 1.440709,  // 45
    103.819953, 1.449096,  // 44
    103.83470, 1.42919,    // 43
    103.83330, 1.41761,    // 23
    103.84480, 1.38166,    // 42
    103.84960, 1.37,       // 2
    103.84810, 1.35059,    // 9
    103.84670, 1.34082,    // 5
    103.84780, 1.33186,    // 41
    103.84380, 1.32038,    // 26
    103.83780, 1.31260,    // 27
    103.83230, 1.30442,    // 28
    103.83870, 1.29994,    // 36
    103.8467, 1.29833,     // 15
    103.8525, 1.29282,     // 14
    103.85210, 1.28397,    // 32
    103.85420, 1.27629,    // 3

    103.94700, 1.32749,    // 39
    103.961571, 1.334554,  // 52
    103.988802, 1.357386,  // 53

    103.820981, 1.265264,  // NE1
    103.83900, 1.28168,    // 29
    103.843251, 1.284269,  // NE4
    103.846819, 1.288651,  // ne5
    103.8467, 1.29833,     // 15
    103.849850, 1.307222,  // NE7
    103.854331, 1.312392,  // NE8
    103.861883, 1.319607,  // NE9
    103.869227, 1.331377,  // NE10
    103.870757, 1.339338,  // NE11
    103.873424, 1.349244,  // NE12
    103.885198, 1.360467,  // NE13
    103.892296, 1.370997,  // NE14
    103.892839, 1.382436,  // NE15
    103.895238, 1.391292,  // NE16
    103.902283, 1.404917,  // PTC

    103.8467, 1.29833,     // CC1
    103.850666, 1.296968,  // cc2
    103.855413, 1.293534,  // cc3
    103.861039, 1.293172,  // cc4
    103.863597, 1.299737,  // cc5
    103.875293, 1.302810,  // cc6
    103.882525, 1.306291,  // cc7
    103.888263, 1.308227,  // cc8
    103.89290, 1.31809,    // 30
    103.890089, 1.326663,  // cc10
    103.887949, 1.335865,  // cc11
    103.879998, 1.342627,  // cc12
    103.873424, 1.349244,  // NE12
    103.863532, 1.352044,  // cc14
    103.84810, 1.35059,    // 9
    103.839539, 1.349070,  // cc16
    103.839504, 1.337756,  // cc17
    103.815474, 1.322585,  // cc19
    103.807443, 1.317289,  // cc20
    103.796193, 1.312119,  // cc21
    103.79030, 1.30726,    // 8
    103.787096, 1.299318,  // cc23
    103.784432, 1.293378,  // cc24
    103.781826, 1.282361,  // cc25
    103.791348, 1.276135,  // cc26
    103.802922, 1.272304,  // cc27
    103.809701, 1.270629,  // cc28
    103.820981, 1.265264,  // NE1
    103.831111, 1.270000,  // CC30 Keppel
    103.836889, 1.272921,  // CC31 Cantonment
    103.846786, 1.273130,  // CC32 Prince Edward Road

    103.861039, 1.293172,  // cc4
    103.859280, 1.282495,  // CE1
    103.85420, 1.27629,    // NS27, CE2

    // 103.954683,	1.356326,
    // 103.961536,	1.341740,
    // 103.962335,	1.335688
    103.762138, 1.378296,  // DT1 Bukit Panjang
    103.764439, 1.369201,  // DT2 Cashew
    103.767410, 1.362345,  // DT3 Hillview
    103.768685, 1.354712,  // DT4 Hume (Future)
    103.775811, 1.341223,  // DT5 Beauty World
    103.783262, 1.335809,  // DT6 King Albert Park
    103.796907, 1.330858,  // DT7 Sixth Avenue
    103.807758, 1.325707,  // DT8 Tan Kah Kee
    103.815747, 1.322559,  // DT9 Botanic Gardens
    103.825461, 1.320192,  // DT10 Stevens
    103.837997, 1.312318,  // DT11 Newton
    103.849186, 1.306806,  // DT12 Little India
    103.852834, 1.303852,  // DT13 Rochor
    103.856149, 1.300181,  // DT14 Bugis
    103.861075, 1.293043,  // DT15 Promenade
    103.859079, 1.281869,  // DT16 Bayfront
    103.852841, 1.279446,  // DT17 Downtown
    103.848608, 1.282054,  // DT18 Telok Ayer
    103.844005, 1.284758,  // DT19 Chinatown
    103.844331, 1.292482,  // DT20 Fort Canning
    103.850278, 1.298611,  // DT21 Bencoolen
    103.855278, 1.305194,  // DT22 Jalan Besar
    103.862889, 1.313667,  // DT23 Bendemeer
    103.871444, 1.321333,  // DT24 Geylang Bahru
    103.883250, 1.326861,  // DT25 Mattar
    103.889989, 1.325842,  // DT26 MacPherson
    103.899958, 1.330011,  // DT27 Ubi
    103.908556, 1.334967,  // DT28 Kaki Bukit
    103.917997, 1.334742,  // DT29 Bedok North
    103.932889, 1.336111,  // DT30 Bedok Reservoir
    103.938361, 1.345556,  // DT31 Tampines West
    103.945222, 1.353278,  // DT32 Tampines
    103.954639, 1.356194,  // DT33 Tampines East
    103.961389, 1.341694,  // DT34 Upper Changi
    103.962319, 1.335383,  // DT35 Expo

    /*103.784411, 1.437025, // TE1 Woodlands North
    103.786311, 1.436889, // TE2 Woodlands
    103.793778, 1.427389, // TE3 Woodlands South
    103.818456, 1.380111, // TE4 Springleaf
    103.832961, 1.369406, // TE5 Lentor
    103.834011, 1.357186, // TE6 Mayflower
    103.837333, 1.342111, // TE7 Bright Hill
    103.834406, 1.329972, // TE8 Upper Thomson
    103.820719, 1.320478, // TE9 Caldecott
    103.825461, 1.320192, // TE11 Stevens
    103.826722, 1.312911, // TE12 Napier
    103.829028, 1.304528, // TE13 Orchard Boulevard
    103.832167, 1.304322, // TE14 Orchard
    103.836889, 1.298222, // TE15 Great World
    103.839361, 1.289111, // TE16 Havelock
    103.844005, 1.284758, // TE17 Chinatown (Outram Park interchange node)
    103.839722, 1.280278, // TE17 Outram Park
    103.844167, 1.274722, // TE18 Maxwell
    103.849167, 1.275556, // TE19 Shenton Way
    103.856944, 1.276389, // TE20 Marina Bay
    103.864722, 1.281389, // TE22 Gardens by the Bay
    103.870833, 1.297778, // TE22A Founders' Memorial (Future)
    103.881667, 1.298611, // TE23 Tanjong Rhu
    103.889444, 1.301389, // TE24 Katong Park
    103.899722, 1.302500, // TE25 Tanjong Katong
    103.905278, 1.305000, // TE26 Marine Parade
    103.914722, 1.308056, // TE27 Marine Terrace
    103.923889, 1.311667, // TE28 Siglap
    103.935833, 1.316389, // TE29 Bayshore
    103.951111, 1.318889, // TE30 Bedok South (Future)
    103.962500, 1.329444, // TE31 Sungei Bedok (Future)*/

    // Thomson-East Coast Line (TEL)
    103.785800, 1.448900,  // TE1 WOODLANDS NORTH
    103.786800, 1.436900,  // TE2 WOODLANDS
    103.793100, 1.427400,  // TE3 WOODLANDS SOUTH
    103.818000, 1.397300,  // TE4 SPRINGLEAF
    103.836500, 1.385500,  // TE5 LENTOR
    103.836800, 1.372000,  // TE6 MAYFLOWER
    103.833200, 1.363300,  // TE7 BRIGHT HILL
    103.832100, 1.354300,  // TE8 UPPER THOMSON
    103.839500, 1.337700,  // TE9 CALDECOTT
    103.835700, 1.328600,  // TE10 MOUNT PLEASANT
    103.826000, 1.320100,  // TE11 STEVENS
    103.819700, 1.306400,  // TE12 NAPIER
    103.823900, 1.302400,  // TE13 ORCHARD BOULEVARD
    103.832900, 1.304000,  // TE14 ORCHARD
    103.833200, 1.293000,  // TE15 GREAT WORLD
    103.833000, 1.288400,  // TE16 HAVELOCK
    103.839100, 1.281900,  // TE17 OUTRAM PARK
    103.844900, 1.280300,  // TE18 MAXWELL
    103.850600, 1.277500,  // TE19 SHENTON WAY
    103.854600, 1.276200,  // TE20 MARINA BAY
    103.861300, 1.274300,  // TE21 MARINA SOUTH
    103.867200, 1.279400,  // TE22 GARDENS BY THE BAY
    103.875600, 1.299400,  // TE23 TANJONG RHU
    103.884400, 1.302800,  // TE24 KATONG PARK
    103.897100, 1.299600,  // TE25 TANJONG KATONG
    103.907300, 1.303000,  // TE26 MARINE PARADE
    103.915700, 1.306400,  // TE27 MARINE TERRACE
    103.928300, 1.311100,  // TE28 SIGLAP
    103.938600, 1.313700,  // TE29 BAYSHORE
    103.950400, 1.317900,  // TE30 BEDOK SOUTH
    103.957900, 1.321700   // TE31 SUNGEI BEDOK
};

const char* mrt_name[] = {
    "EW1 Pasir Ris",
    "EW2 Tampines",
    "EW3 Simei",
    "EW4 Tanah Merah",
    "EW5 Bedok",
    "EW6 Kembangan",
    "EW7 Eunos",
    "EW8 Paya Lebar",
    "EW9 Aljunied",
    "EW10 Kallang",
    "EW11 Lavender",
    "EW12 Bugis",
    "EW13 City Hall",
    "EW14 Raffles Place",
    "EW15 Tanjong Pagar",
    "EW16 Outram Park",
    "EW17 Tiong Bahru",
    "EW18 Redhill",
    "EW19 Queenstown",
    "EW20 Commonwealth",
    "EW21 Buona Vista",
    "EW22 Dover",
    "EW23 Clementi",
    "EW24 Jurong East",
    "EW25 Chinese Garden",
    "EW26 Lakeside",
    "EW27 Boon Lay",
    "EW28 Pioneer",
    "EW29 Joo Koon",

    "NS1 Jurong East",
    "NS2 Bukit Batok",
    "NS3 Bukit Gombak",
    "NS4 Choa Chu Kang",
    "NS5 Yew Tee",
    "NS7 Kranji",
    "NS8 Marsiling",
    "NS9 Woodlands",
    "NS10 Admiralty",
    "NS11 Sembawang",
    "NS12 Yishun",
    "NS14 Khatib",
    "NS15 Yio Chu Kang",
    "NS16 Ang Mo Kio",
    "NS17 Bishan",
    "NS18 Braddell",
    "NS19 Toa Payoh",
    "NS20 Novena",
    "NS21 Newton",
    "NS22 Orchard",
    "NS23 Somerset",
    "NS24 Dhoby Ghaut",
    "NS25 City Hall",
    "NS26 Raffles Place",
    "NS27 Marina Bay",

    "CG Tanah Merah",
    "CG1 Expo",
    "CG2 Changi Airport",

    "NE1 HarbourFront",
    "NE3 Outram Park",
    "NE4 Chinatown",
    "NE5 Clarke Quay",
    "NE6 Dhoby Ghaut",
    "NE7 Little India",
    "NE8 Farrer Park",
    "NE9 Boon Keng",
    "NE10 Potong Pasir",
    "NE11 Woodleigh",
    "NE12 Serangoon",
    "NE13 Kovan",
    "NE14 Hougang",
    "NE15 Buangkok",
    "NE16 Sengkang",
    "NE17 Punggol",

    "CC1 Dhoby Ghaut",
    "CC2 Bras Basah",
    "CC3 Esplanade",
    "CC4 Promenade",
    "CC5 Nicoll Highway",
    "CC6 Stadium",
    "CC7 Mountbatten",
    "CC8 Dakota",
    "CC9 Paya Lebar",
    "CC10 MacPherson",
    "CC11 Tai Seng",
    "CC12 Bartley",
    "CC13 Serangoon",
    "CC14 Lorong Chuan",
    "CC15 Bishan",
    "CC16 Marymount",
    "CC17 Caldecott",
    // CC18 Bukit Brown (not open)
    "CC19 Botanic Gardens",
    "CC20 Farrer Road",
    "CC21 Holland Village",
    "CC22 Buona Vista",
    "CC23 One North",
    "CC24 Kent Ridge",
    "CC25 Haw Par Villa",
    "CC26 Pasir Panjang",
    "CC27 Labrador Park",
    "CC28 Telok Blangah",
    "CC29 HarbourFront",
    "CC30 Keppel",
    "CC31 Cantonment",
    "CC32 Prince Edward Road",

    "CC4 Promenade",
    "CC34 Bayfront",
    "CC33 Marina Bay",

    "DT1 Bukit Panjang",
    "DT2 Cashew",
    "DT3 Hillview",
    "DT4 Hume",
    "DT5 Beauty World",
    "DT6 King Albert Park",
    "DT7 Sixth Avenue",
    "DT8 Tan Kah Kee",
    "DT9 Botanic Gardens",
    "DT10 Stevens",
    "DT11 Newton",
    "DT12 Little India",
    "DT13 Rochor",
    "DT14 Bugis",
    "DT15 Promenade",
    "DT16 Bayfront",
    "DT17 Downtown",
    "DT18 Telok Ayer",
    "DT19 Chinatown",
    "DT20 Fort Canning",
    "DT21 Bencoolen",
    "DT22 Jalan Besar",
    "DT23 Bendemeer",
    "DT24 Geylang Bahru",
    "DT25 Mattar",
    "DT26 MacPherson",
    "DT27 Ubi",
    "DT28 Kaki Bukit",
    "DT29 Bedok North",
    "DT30 Bedok Reservoir",
    "DT31 Tampines West",
    "DT32 Tampines",
    "DT33 Tampines East",
    "DT34 Upper Changi",
    "DT35 Expo",

    "TE1 Woodlands North",
    "TE2 Woodlands",
    "TE3 Woodlands South",
    "TE4 Springleaf",
    "TE5 Lentor",
    "TE6 Mayflower",
    "TE7 Bright Hill",
    "TE8 Upper Thomson",
    "TE9 Caldecott",
    "TE10 Mount Pleasant",
    "TE11 Stevens",
    "TE12 Napier",
    "TE13 Orchard Boulevard",
    "TE14 Orchard",
    "TE15 Great World",
    "TE16 Havelock",
    "TE17 Outram Park",
    "TE18 Maxwell",
    "TE19 Shenton Way",
    "TE20 Marina Bay",
    "TE21 Marina South",
    "TE22 Gardens By The Bay",
    "TE23 Tanjong Rhu",
    "TE24 Katong Park",
    "TE25 Tanjong Katong",
    "TE26 Marine Parade",
    "TE27 Marine Terrace",
    "TE28 Siglap",
    "TE29 Bayshore",
    "TE30 Bedok South",
    "TE31 Sungei Bedok",
};

/*

MRTALJ  ALJUNIED MRT            N 01.31637     E103.88270      265939201
MRTAMK  ANG MO KIO MRT          N 01.37000     E103.84960      265939201
MRTBAY  MARINA BAY MRT          N 01.27629     E103.85420      265939201
MRTBDK  BEDOK MRT               N 01.32405     E103.93020      265939201
MRTBDL  BRADDELL MRT            N 01.34082     E103.84670      265939201
MRTBGS  BUGIS MRT               N 01.30006     E103.85590      265939201
MRTBLY  BOON LAY MRT            N 01.33948     E103.70580      265939201
MRTBNV  BUONA VISTA MRT         N 01.30726     E103.79030      265939201
MRTBSN  BISHAN MRT              N 01.35059     E103.84810      265939201
MRTBTK  BKT. BATOK MRT          N 01.34924     E103.74950      265939201
MRTCCK  CHOA CHU KANG           N 01.38505     E103.74430      265939201
MRTCMT  CLEMENTI MRT            N 01.31493     E103.76530      265939201
MRTCMW  COMMONWEALTH MRT        N 01.30245     E103.79850      265939201
MRTCTH  CITY HALL MRT           N 01.29282     E103.85250      265939201
MRTDBG  DHOBY GHAUT MRT         N 01.29833     E103.84670      265939201
MRTDP1  BISHAN MRT DEPOT        N 01.35666     E103.85340      265939201
MRTDP2  CHANGI MRT DEPOT        N 01.33093     E103.95550      265939201
MRTDP3  ULU PANDAN DEPOT        N 01.33083     E103.76120      265939201
MRTENS  EUNOS MRT               N 01.31969     E103.90330      265939201
MRTGBK  BKT. GOMBAK MRT         N 01.35955     E103.75180      265939201
MRTGDN  CHINESE GARDENS         N 01.34209     E103.73260      265939201
MRTJGE  JURONG EAST MRT         N 01.33237     E103.74240      265939201
MRTKBG  KEMBANGAN MRT           N 01.32107     E103.91290      265939201
MRTKJI  KRANJI MRT              N 01.42555     E103.76180      265939201
MRTKLG  KALLANG MRT             N 01.31154     E103.87180      265939201
MRTKTB  KHATIB MRT              N 01.41761     E103.83330      265939201
MRTLKE  LAKESIDE MRT            N 01.34455     E103.72100      265939201
MRTLVD  LAVENDER MRT            N 01.30752     E103.86320      265939201
MRTNOV  NOVENA MRT              N 01.32038     E103.84380      265939201
MRTNWT  NEWTON MRT              N 01.31260     E103.83780      265939201
MRTORD  ORCHARD MRT             N 01.30442     E103.83230      265939201
MRTOTR  OUTRAM MRT              N 01.28168     E103.83900      265939201
MRTPYL  PAYA LEBAR MRT          N 01.31809     E103.89290      265939201
MRTQTN  QUEENSTOWN MRT          N 01.29419     E103.80650      265939201
MRTRAF  RAFFLES PL. MRT         N 01.28397     E103.85210      265939201
MRTRED  REDHILL MRT             N 01.28971     E103.81680      265939201
MRTRIS  PASIR RIS MRT           N 01.37303     E103.94910      265939201
MRTSMI  SIMEI MRT               N 01.34371     E103.95310      265939201
MRTSMT  SOMERSET MRT            N 01.29994     E103.83870      265939201
MRTTGB  TIONG BAHRU MRT         N 01.28639     E103.82680      265939201
MRTTGP  TJG. PAGAR MRT          N 01.27696     E103.84590      265939201
MRTTNM  TANAH MERAH MRT         N 01.32749     E103.94700      265939201
MRTTPN  TAMPINES MRT            N 01.35372     E103.94480      265939201
MRTTPY  TOA PAYOH MRT           N 01.33186     E103.84780      265939201
MRTYCK  YIO CHU KANG MRT        N 01.38166     E103.84480      265939201
MRTYSN  YISHUN MRT              N 01.42919     E103.83470      265939201
*/
#endif