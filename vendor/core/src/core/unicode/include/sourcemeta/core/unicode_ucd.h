#ifndef SOURCEMETA_CORE_UNICODE_UCD_H_
#define SOURCEMETA_CORE_UNICODE_UCD_H_

#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

/// @ingroup unicode
/// Each entry maps a `JoiningType` enum name to its UCD short alias.
#define SOURCEMETA_CORE_JOINING_TYPE_LIST(X)                                   \
  X(NonJoining, "U")                                                           \
  X(Transparent, "T")                                                          \
  X(LeftJoining, "L")                                                          \
  X(RightJoining, "R")                                                         \
  X(DualJoining, "D")                                                          \
  X(JoinCausing, "C")

/// @ingroup unicode
/// The joining type of a Unicode codepoint per UAX #44. See
/// https://www.unicode.org/reports/tr44/ for the property's definition.
enum class JoiningType : std::uint8_t {
#define SOURCEMETA_CORE_UCD_ENUM_ENTRY(name, alias) name,
  SOURCEMETA_CORE_JOINING_TYPE_LIST(SOURCEMETA_CORE_UCD_ENUM_ENTRY)
#undef SOURCEMETA_CORE_UCD_ENUM_ENTRY
};

/// @ingroup unicode
/// Each entry maps a `BidiClass` enum name to its UCD short alias.
#define SOURCEMETA_CORE_BIDI_CLASS_LIST(X)                                     \
  X(LeftToRight, "L")                                                          \
  X(RightToLeft, "R")                                                          \
  X(ArabicLetter, "AL")                                                        \
  X(EuropeanNumber, "EN")                                                      \
  X(EuropeanSeparator, "ES")                                                   \
  X(EuropeanTerminator, "ET")                                                  \
  X(ArabicNumber, "AN")                                                        \
  X(CommonSeparator, "CS")                                                     \
  X(NonspacingMark, "NSM")                                                     \
  X(BoundaryNeutral, "BN")                                                     \
  X(ParagraphSeparator, "B")                                                   \
  X(SegmentSeparator, "S")                                                     \
  X(WhiteSpace, "WS")                                                          \
  X(OtherNeutral, "ON")                                                        \
  X(LeftToRightEmbedding, "LRE")                                               \
  X(LeftToRightOverride, "LRO")                                                \
  X(RightToLeftEmbedding, "RLE")                                               \
  X(RightToLeftOverride, "RLO")                                                \
  X(PopDirectionalFormat, "PDF")                                               \
  X(LeftToRightIsolate, "LRI")                                                 \
  X(RightToLeftIsolate, "RLI")                                                 \
  X(FirstStrongIsolate, "FSI")                                                 \
  X(PopDirectionalIsolate, "PDI")

/// @ingroup unicode
/// The bidirectional class of a Unicode codepoint per UAX #44. See
/// https://www.unicode.org/reports/tr44/ for the property's definition.
enum class BidiClass : std::uint8_t {
#define SOURCEMETA_CORE_UCD_ENUM_ENTRY(name, alias) name,
  SOURCEMETA_CORE_BIDI_CLASS_LIST(SOURCEMETA_CORE_UCD_ENUM_ENTRY)
#undef SOURCEMETA_CORE_UCD_ENUM_ENTRY
};

/// @ingroup unicode
/// Each entry maps a `UnicodeScript` enum name to its UCD long alias.
/// Per UAX #24 §1.4, `Katakana_Or_Hiragana` only appears in the
/// `Script_Extensions` property and never in the `Script` property itself.
#define SOURCEMETA_CORE_UNICODE_SCRIPT_LIST(X)                                 \
  X(Adlam, "Adlam")                                                            \
  X(Ahom, "Ahom")                                                              \
  X(AnatolianHieroglyphs, "Anatolian_Hieroglyphs")                             \
  X(Arabic, "Arabic")                                                          \
  X(Armenian, "Armenian")                                                      \
  X(Avestan, "Avestan")                                                        \
  X(Balinese, "Balinese")                                                      \
  X(Bamum, "Bamum")                                                            \
  X(BassaVah, "Bassa_Vah")                                                     \
  X(Batak, "Batak")                                                            \
  X(Bengali, "Bengali")                                                        \
  X(BeriaErfe, "Beria_Erfe")                                                   \
  X(Bhaiksuki, "Bhaiksuki")                                                    \
  X(Bopomofo, "Bopomofo")                                                      \
  X(Brahmi, "Brahmi")                                                          \
  X(Braille, "Braille")                                                        \
  X(Buginese, "Buginese")                                                      \
  X(Buhid, "Buhid")                                                            \
  X(CanadianAboriginal, "Canadian_Aboriginal")                                 \
  X(Carian, "Carian")                                                          \
  X(CaucasianAlbanian, "Caucasian_Albanian")                                   \
  X(Chakma, "Chakma")                                                          \
  X(Cham, "Cham")                                                              \
  X(Cherokee, "Cherokee")                                                      \
  X(Chorasmian, "Chorasmian")                                                  \
  X(Common, "Common")                                                          \
  X(Coptic, "Coptic")                                                          \
  X(Cuneiform, "Cuneiform")                                                    \
  X(Cypriot, "Cypriot")                                                        \
  X(CyproMinoan, "Cypro_Minoan")                                               \
  X(Cyrillic, "Cyrillic")                                                      \
  X(Deseret, "Deseret")                                                        \
  X(Devanagari, "Devanagari")                                                  \
  X(DivesAkuru, "Dives_Akuru")                                                 \
  X(Dogra, "Dogra")                                                            \
  X(Duployan, "Duployan")                                                      \
  X(EgyptianHieroglyphs, "Egyptian_Hieroglyphs")                               \
  X(Elbasan, "Elbasan")                                                        \
  X(Elymaic, "Elymaic")                                                        \
  X(Ethiopic, "Ethiopic")                                                      \
  X(Garay, "Garay")                                                            \
  X(Georgian, "Georgian")                                                      \
  X(Glagolitic, "Glagolitic")                                                  \
  X(Gothic, "Gothic")                                                          \
  X(Grantha, "Grantha")                                                        \
  X(Greek, "Greek")                                                            \
  X(Gujarati, "Gujarati")                                                      \
  X(GunjalaGondi, "Gunjala_Gondi")                                             \
  X(Gurmukhi, "Gurmukhi")                                                      \
  X(GurungKhema, "Gurung_Khema")                                               \
  X(Han, "Han")                                                                \
  X(Hangul, "Hangul")                                                          \
  X(HanifiRohingya, "Hanifi_Rohingya")                                         \
  X(Hanunoo, "Hanunoo")                                                        \
  X(Hatran, "Hatran")                                                          \
  X(Hebrew, "Hebrew")                                                          \
  X(Hiragana, "Hiragana")                                                      \
  X(ImperialAramaic, "Imperial_Aramaic")                                       \
  X(Inherited, "Inherited")                                                    \
  X(InscriptionalPahlavi, "Inscriptional_Pahlavi")                             \
  X(InscriptionalParthian, "Inscriptional_Parthian")                           \
  X(Javanese, "Javanese")                                                      \
  X(Kaithi, "Kaithi")                                                          \
  X(Kannada, "Kannada")                                                        \
  X(Katakana, "Katakana")                                                      \
  X(Kawi, "Kawi")                                                              \
  X(KayahLi, "Kayah_Li")                                                       \
  X(Kharoshthi, "Kharoshthi")                                                  \
  X(KhitanSmallScript, "Khitan_Small_Script")                                  \
  X(Khmer, "Khmer")                                                            \
  X(Khojki, "Khojki")                                                          \
  X(Khudawadi, "Khudawadi")                                                    \
  X(KiratRai, "Kirat_Rai")                                                     \
  X(Lao, "Lao")                                                                \
  X(Latin, "Latin")                                                            \
  X(Lepcha, "Lepcha")                                                          \
  X(Limbu, "Limbu")                                                            \
  X(LinearA, "Linear_A")                                                       \
  X(LinearB, "Linear_B")                                                       \
  X(Lisu, "Lisu")                                                              \
  X(Lycian, "Lycian")                                                          \
  X(Lydian, "Lydian")                                                          \
  X(Mahajani, "Mahajani")                                                      \
  X(Makasar, "Makasar")                                                        \
  X(Malayalam, "Malayalam")                                                    \
  X(Mandaic, "Mandaic")                                                        \
  X(Manichaean, "Manichaean")                                                  \
  X(Marchen, "Marchen")                                                        \
  X(MasaramGondi, "Masaram_Gondi")                                             \
  X(Medefaidrin, "Medefaidrin")                                                \
  X(MeeteiMayek, "Meetei_Mayek")                                               \
  X(MendeKikakui, "Mende_Kikakui")                                             \
  X(MeroiticCursive, "Meroitic_Cursive")                                       \
  X(MeroiticHieroglyphs, "Meroitic_Hieroglyphs")                               \
  X(Miao, "Miao")                                                              \
  X(Modi, "Modi")                                                              \
  X(Mongolian, "Mongolian")                                                    \
  X(Mro, "Mro")                                                                \
  X(Multani, "Multani")                                                        \
  X(Myanmar, "Myanmar")                                                        \
  X(Nabataean, "Nabataean")                                                    \
  X(NagMundari, "Nag_Mundari")                                                 \
  X(Nandinagari, "Nandinagari")                                                \
  X(NewTaiLue, "New_Tai_Lue")                                                  \
  X(Newa, "Newa")                                                              \
  X(Nko, "Nko")                                                                \
  X(Nushu, "Nushu")                                                            \
  X(NyiakengPuachueHmong, "Nyiakeng_Puachue_Hmong")                            \
  X(Ogham, "Ogham")                                                            \
  X(OlChiki, "Ol_Chiki")                                                       \
  X(OlOnal, "Ol_Onal")                                                         \
  X(OldHungarian, "Old_Hungarian")                                             \
  X(OldItalic, "Old_Italic")                                                   \
  X(OldNorthArabian, "Old_North_Arabian")                                      \
  X(OldPermic, "Old_Permic")                                                   \
  X(OldPersian, "Old_Persian")                                                 \
  X(OldSogdian, "Old_Sogdian")                                                 \
  X(OldSouthArabian, "Old_South_Arabian")                                      \
  X(OldTurkic, "Old_Turkic")                                                   \
  X(OldUyghur, "Old_Uyghur")                                                   \
  X(Oriya, "Oriya")                                                            \
  X(Osage, "Osage")                                                            \
  X(Osmanya, "Osmanya")                                                        \
  X(PahawhHmong, "Pahawh_Hmong")                                               \
  X(Palmyrene, "Palmyrene")                                                    \
  X(PauCinHau, "Pau_Cin_Hau")                                                  \
  X(PhagsPa, "Phags_Pa")                                                       \
  X(Phoenician, "Phoenician")                                                  \
  X(PsalterPahlavi, "Psalter_Pahlavi")                                         \
  X(Rejang, "Rejang")                                                          \
  X(Runic, "Runic")                                                            \
  X(Samaritan, "Samaritan")                                                    \
  X(Saurashtra, "Saurashtra")                                                  \
  X(Sharada, "Sharada")                                                        \
  X(Shavian, "Shavian")                                                        \
  X(Siddham, "Siddham")                                                        \
  X(Sidetic, "Sidetic")                                                        \
  X(SignWriting, "SignWriting")                                                \
  X(Sinhala, "Sinhala")                                                        \
  X(Sogdian, "Sogdian")                                                        \
  X(SoraSompeng, "Sora_Sompeng")                                               \
  X(Soyombo, "Soyombo")                                                        \
  X(Sundanese, "Sundanese")                                                    \
  X(Sunuwar, "Sunuwar")                                                        \
  X(SylotiNagri, "Syloti_Nagri")                                               \
  X(Syriac, "Syriac")                                                          \
  X(Tagalog, "Tagalog")                                                        \
  X(Tagbanwa, "Tagbanwa")                                                      \
  X(TaiLe, "Tai_Le")                                                           \
  X(TaiTham, "Tai_Tham")                                                       \
  X(TaiViet, "Tai_Viet")                                                       \
  X(TaiYo, "Tai_Yo")                                                           \
  X(Takri, "Takri")                                                            \
  X(Tamil, "Tamil")                                                            \
  X(Tangsa, "Tangsa")                                                          \
  X(Tangut, "Tangut")                                                          \
  X(Telugu, "Telugu")                                                          \
  X(Thaana, "Thaana")                                                          \
  X(Thai, "Thai")                                                              \
  X(Tibetan, "Tibetan")                                                        \
  X(Tifinagh, "Tifinagh")                                                      \
  X(Tirhuta, "Tirhuta")                                                        \
  X(Todhri, "Todhri")                                                          \
  X(TolongSiki, "Tolong_Siki")                                                 \
  X(Toto, "Toto")                                                              \
  X(TuluTigalari, "Tulu_Tigalari")                                             \
  X(Ugaritic, "Ugaritic")                                                      \
  X(Unknown, "Unknown")                                                        \
  X(Vai, "Vai")                                                                \
  X(Vithkuqi, "Vithkuqi")                                                      \
  X(Wancho, "Wancho")                                                          \
  X(WarangCiti, "Warang_Citi")                                                 \
  X(Yezidi, "Yezidi")                                                          \
  X(Yi, "Yi")                                                                  \
  X(ZanabazarSquare, "Zanabazar_Square")                                       \
  X(KatakanaOrHiragana, "Katakana_Or_Hiragana")

/// @ingroup unicode
/// The script of a Unicode codepoint per UAX #24. See
/// https://www.unicode.org/reports/tr24/ for the property's definition.
enum class UnicodeScript : std::uint8_t {
#define SOURCEMETA_CORE_UCD_ENUM_ENTRY(name, alias) name,
  SOURCEMETA_CORE_UNICODE_SCRIPT_LIST(SOURCEMETA_CORE_UCD_ENUM_ENTRY)
#undef SOURCEMETA_CORE_UCD_ENUM_ENTRY
};

/// @ingroup unicode
/// Each entry maps an `NFCQuickCheck` enum name to its UCD short alias.
#define SOURCEMETA_CORE_NFC_QUICK_CHECK_LIST(X)                                \
  X(Yes, "Y")                                                                  \
  X(No, "N")                                                                   \
  X(Maybe, "M")

/// @ingroup unicode
/// The NFC quick-check result for a Unicode codepoint per UAX #15.
/// See https://www.unicode.org/reports/tr15/ for the property's definition.
enum class NFCQuickCheck : std::uint8_t {
#define SOURCEMETA_CORE_UCD_ENUM_ENTRY(name, alias) name,
  SOURCEMETA_CORE_NFC_QUICK_CHECK_LIST(SOURCEMETA_CORE_UCD_ENUM_ENTRY)
#undef SOURCEMETA_CORE_UCD_ENUM_ENTRY
};

} // namespace sourcemeta::core

#endif
