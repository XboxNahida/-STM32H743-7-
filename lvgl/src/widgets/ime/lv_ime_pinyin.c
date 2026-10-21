/**
 * @file lv_ime_pinyin.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_ime_pinyin_private.h"
#include "../../core/lv_obj_class_private.h"
#if LV_USE_IME_PINYIN != 0

#include "../../lvgl.h"
#include "../../core/lv_global.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS (&lv_ime_pinyin_class)
#define cand_len LV_GLOBAL_DEFAULT()->ime_cand_len
#define IME_LANGUAGE_ID     0U
#define IME_PAGE_PREV_ID    1U
#define IME_PREEDIT_ID      2U
#define IME_CHINESE_FIRST_ID 3U
#define IME_PAGE_NEXT_ID    (IME_CHINESE_FIRST_ID + LV_IME_PINYIN_CAND_TEXT_NUM)
#define IME_PANEL_BUTTON_COUNT (IME_PAGE_NEXT_ID + 1U)
#define IME_PANEL_MAP_COUNT (IME_PANEL_BUTTON_COUNT + 1U)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_style_change_event(lv_event_t * e);
static void lv_ime_pinyin_kb_event(lv_event_t * e);
static void lv_ime_pinyin_cand_panel_event(lv_event_t * e);

static void init_pinyin_dict(lv_obj_t * obj, const lv_pinyin_dict_t * dict);
static void pinyin_input_proc(lv_obj_t * obj);
static void pinyin_page_proc(lv_obj_t * obj, uint16_t btn);
static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num);
static void pinyin_ime_clear_data(lv_obj_t * obj);
static void pinyin_update_preedit_display(lv_ime_pinyin_t * pinyin_ime);
static void pinyin_update_language_state(lv_ime_pinyin_t * pinyin_ime);
static void pinyin_update_panel_visibility(lv_ime_pinyin_t * pinyin_ime);
static void pinyin_update_keyboard_target(lv_ime_pinyin_t * pinyin_ime);
static void pinyin_fill_candidate_page(lv_ime_pinyin_t * pinyin_ime);

#if LV_IME_PINYIN_USE_K9_MODE
    static void pinyin_k9_init_data(lv_obj_t * obj);
    static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[]);
    static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str);
    static void pinyin_k9_fill_cand(lv_obj_t * obj);
    static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_ime_pinyin_class = {
    .constructor_cb = lv_ime_pinyin_constructor,
    .destructor_cb  = lv_ime_pinyin_destructor,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .group_def      = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size  = sizeof(lv_ime_pinyin_t),
    .base_class     = &lv_obj_class,
    .name = "lv_ime_pinyin",
};

#if LV_IME_PINYIN_USE_K9_MODE
static const char * lv_btnm_def_pinyin_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 21] = {\
                                                                                      ",\0", "123\0",  "abc \0", "def\0",  LV_SYMBOL_BACKSPACE"\0", "\n\0",
                                                                                      ".\0", "ghi\0", "jkl\0", "mno\0",  LV_SYMBOL_KEYBOARD"\0", "\n\0",
                                                                                      "?\0", "pqrs\0", "tuv\0", "wxyz\0",  LV_SYMBOL_NEW_LINE"\0", "\n\0",
                                                                                      LV_SYMBOL_LEFT"\0", "\0"
                                                                                     };

static lv_buttonmatrix_ctrl_t default_kb_ctrl_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 17] = {
    LV_BUTTONMATRIX_CTRL_WIDTH_1
};
static char   lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 2][LV_IME_PINYIN_K9_MAX_INPUT] = {0};
#endif

static char   lv_pinyin_cand_str[LV_IME_PINYIN_CAND_TEXT_NUM][4];
static char * lv_btnm_def_pinyin_sel_map[IME_PANEL_MAP_COUNT];
static lv_buttonmatrix_ctrl_t lv_btnm_def_pinyin_sel_ctrl[IME_PANEL_BUTTON_COUNT];

#if LV_IME_PINYIN_USE_DEFAULT_DICT
static const lv_pinyin_dict_t lv_ime_pinyin_def_dict[] = {
    { "a", "啊" },
    { "ai", "愛" },
    { "an", "安暗案" },
    { "ba", "吧把爸八" },
    { "bai", "百白敗" },
    { "ban", "半般辦" },
    { "bang", "旁" },
    { "bao", "保薄包報" },
    { "bei", "被背悲北杯備" },
    { "ben", "本" },
    { "bi", "必比避鼻彼筆秘閉" },
    { "bian", "便邊變変辺" },
    { "biao", "表標" },
    { "bie", "別" },
    { "bing", "病並氷" },
    { "bo", "波薄泊" },
    { "bu", "不布步部捕補歩" },
    { "ca", "察" },
    { "cai", "才材菜財採" },
    { "can", "参残參" },
    { "ce", "策側" },
    { "ceng", "曾" },
    { "cha", "差查茶" },
    { "chai", "差" },
    { "chan", "產産單" },
    { "chang", "場廠" },
    { "chao", "超朝" },
    { "che", "車" },
    { "cheng", "成程乗" },
    { "chi", "尺吃持赤池遅歯" },
    { "chong", "充种重種" },
    { "chu", "出初楚触處処" },
    { "chuan", "川船傳" },
    { "chuang", "創窓" },
    { "chun", "春" },
    { "ci", "此次辞差" },
    { "cong", "從従" },
    { "cu", "卒" },
    { "cun", "存村" },
    { "cuo", "錯" },
    { "da", "大打答達" },
    { "dai", "代待帯帶貸" },
    { "dan", "但担擔誕單単" },
    { "dang", "当党當黨" },
    { "dao", "到道盗導島辺" },
    { "de", "的得" },
    { "dei", "" },
    { "deng", "等" },
    { "di", "地得低底弟第締" },
    { "dian", "点电店點電" },
    { "diao", "調" },
    { "ding", "定町" },
    { "dong", "冬東動働凍" },
    { "du", "独度都渡読" },
    { "duan", "段断短斷" },
    { "dui", "對対" },
    { "duo", "多駄" },
    { "e", "嗯悪" },
    { "en", "嗯" },
    { "er", "而耳二兒" },
    { "fa", "乏法發発髪" },
    { "fan", "反返犯番仮販飯範払" },
    { "fang", "方放房坊訪" },
    { "fei", "非飛費" },
    { "fen", "分份" },
    { "feng", "風豐" },
    { "fou", "否不" },
    { "fu", "父夫富服符付附府幅婦復複負払" },
    { "gai", "改概該" },
    { "gan", "甘感敢" },
    { "gang", "港剛" },
    { "gao", "告高" },
    { "ge", "各格歌革割個" },
    { "gei", "給" },
    { "gen", "跟根" },
    { "geng", "更" },
    { "gong", "工共供功公" },
    { "gou", "夠構溝" },
    { "gu", "古故鼓" },
    { "guai", "掛" },
    { "guan", "官管慣館觀関關" },
    { "guang", "光広" },
    { "gui", "規帰" },
    { "guo", "果国裏菓國過" },
    { "hai", "孩海害還" },
    { "han", "寒漢" },
    { "hang", "航行" },
    { "hao", "好号" },
    { "he", "合和喝何荷" },
    { "hei", "黒" },
    { "hen", "很" },
    { "heng", "行横" },
    { "hou", "厚喉候後" },
    { "hu", "乎呼湖護" },
    { "hua", "化画花話畫劃" },
    { "huai", "壊劃" },
    { "huan", "緩環歡還換" },
    { "huang", "黄" },
    { "hui", "回会慧絵揮會" },
    { "hun", "混婚" },
    { "huo", "活或火獲" },
    { "i", "" },
    { "ji", "己计及机既急季寄技即集基祭系奇紀積計記済幾際極繼績機濟" },
    { "jia", "家加價" },
    { "jian", "件建健肩見減間検簡漸" },
    { "jiang", "降強講將港" },
    { "jiao", "叫教交角覚覺較學" },
    { "jie", "介借接姐皆届界解結階節價" },
    { "jin", "今近禁金僅進" },
    { "jing", "京境景静精經経" },
    { "jiu", "就久九酒究" },
    { "ju", "句具局居決挙據舉" },
    { "jue", "角覚覺" },
    { "jun", "均" },
    { "kai", "開" },
    { "kan", "看刊" },
    { "kang", "康" },
    { "kao", "考" },
    { "ke", "可刻科克客渇課" },
    { "ken", "肯" },
    { "kong", "空控" },
    { "kou", "口" },
    { "ku", "苦庫" },
    { "kuai", "快塊会會" },
    { "kuang", "況" },
    { "kun", "困" },
    { "kuo", "括拡適" },
    { "la", "拉啦落" },
    { "lai", "来來頼" },
    { "lao", "老絡落" },
    { "le", "了楽樂" },
    { "lei", "類" },
    { "leng", "冷" },
    { "li", "力立利理例礼離麗裡勵歷" },
    { "lian", "連練臉聯" },
    { "liang", "良量涼兩両" },
    { "liao", "料" },
    { "lie", "列" },
    { "lin", "林隣賃" },
    { "ling", "另令領" },
    { "liu", "六留流" },
    { "lu", "律路録緑陸履慮" },
    { "lv", "旅" },
    { "lun", "輪論" },
    { "luo", "落絡" },
    { "ma", "媽嗎嘛" },
    { "mai", "買売" },
    { "man", "滿" },
    { "mang", "忙" },
    { "mao", "毛猫貿" },
    { "me", "麼" },
    { "mei", "美妹每沒毎媒" },
    { "men", "們" },
    { "mi", "米密秘" },
    { "mian", "免面勉眠" },
    { "miao", "描" },
    { "min", "民皿" },
    { "ming", "命明名" },
    { "mo", "末模麼" },
    { "mou", "某" },
    { "mu", "母木目模" },
    { "na", "那哪拿內南" },
    { "nan", "男南難" },
    { "nao", "腦" },
    { "ne", "那哪呢" },
    { "nei", "内那哪內" },
    { "neng", "能" },
    { "ni", "你妳呢" },
    { "nian", "年念" },
    { "niang", "娘" },
    { "nin", "您" },
    { "ning", "凝" },
    { "niu", "牛" },
    { "nong", "農濃" },
    { "nu", "女努" },
    { "nuan", "暖" },
    { "o", "" },
    { "ou", "歐" },
    { "pa", "怕" },
    { "pai", "迫派排" },
    { "pan", "判番" },
    { "pang", "旁" },
    { "pei", "配" },
    { "peng", "朋" },
    { "pi", "疲否" },
    { "pian", "片便" },
    { "pin", "品貧" },
    { "ping", "平評" },
    { "po", "迫破泊頗" },
    { "pu", "普僕" },
    { "qi", "起其奇七气期泣企妻契気" },
    { "qian", "嵌浅千前鉛錢針" },
    { "qiang", "強將" },
    { "qiao", "橋繰" },
    { "qie", "且切契" },
    { "qin", "寝勤親" },
    { "qing", "青清情晴輕頃請軽" },
    { "qiu", "求秋球" },
    { "qu", "去取趣曲區" },
    { "quan", "全犬券" },
    { "que", "缺確卻" },
    { "ran", "然" },
    { "rang", "讓" },
    { "re", "熱" },
    { "ren", "人任認" },
    { "reng", "仍" },
    { "ri", "日" },
    { "rong", "容" },
    { "rou", "弱若肉" },
    { "ru", "如入" },
    { "ruan", "軟" },
    { "sai", "賽" },
    { "san", "三" },
    { "sao", "騒繰" },
    { "se", "色" },
    { "sen", "森" },
    { "sha", "砂" },
    { "shan", "善山單" },
    { "shang", "上尚商" },
    { "shao", "少紹" },
    { "shaung", "雙" },
    { "she", "社射設捨渉" },
    { "shei", "誰" },
    { "shen", "什申深甚身伸沈神" },
    { "sheng", "生声昇勝乗聲" },
    { "shi", "是失示食时事式十石施使世实史室市始柿氏士仕拭時視師試適実實識" },
    { "shou", "手首守受授" },
    { "shu", "束数暑殊樹書屬輸術" },
    { "shui", "水説說誰" },
    { "shuo", "数説說" },
    { "si", "思寺司四私似死価" },
    { "song", "送" },
    { "su", "速宿素蘇訴" },
    { "suan", "算酸" },
    { "sui", "隨雖歲歳" },
    { "sun", "孫" },
    { "suo", "所" },
    { "ta", "她他它牠" },
    { "tai", "太台態臺" },
    { "tan", "探談曇" },
    { "tang", "糖" },
    { "tao", "桃逃套討" },
    { "te", "特" },
    { "ti", "体提替題體戻" },
    { "tian", "天田" },
    { "tiao", "条條調" },
    { "tie", "鉄" },
    { "ting", "停庭聽町" },
    { "tong", "同童通痛统統" },
    { "tou", "投透頭" },
    { "tu", "土徒茶図" },
    { "tuan", "團" },
    { "tui", "推退" },
    { "tuo", "脱駄" },
    { "u", "" },
    { "v", "" },
    { "wai", "外" },
    { "wan", "完万玩晩腕灣" },
    { "wang", "忘望亡往網" },
    { "wei", "危位未味委為謂維違圍" },
    { "wen", "文温問聞" },
    { "wo", "我" },
    { "wu", "午物五無屋亡鳥務汚" },
    { "xi", "夕息西洗喜系昔席希析嬉膝細習係" },
    { "xia", "下夏狭暇" },
    { "xian", "先限嫌洗現見線顯" },
    { "xiang", "向相香像想象降項詳響" },
    { "xiao", "小笑消效校削咲" },
    { "xie", "写携些解邪械協謝寫契" },
    { "xin", "心信新辛" },
    { "xing", "行形性幸型星興" },
    { "xiong", "兄胸" },
    { "xiu", "休秀修" },
    { "xu", "須需許續緒続" },
    { "xuan", "選懸" },
    { "xue", "学雪削靴學" },
    { "xun", "訓訊" },
    { "ya", "呀押壓" },
    { "yan", "言顔研煙嚴厳験驗塩" },
    { "yang", "央洋陽樣様" },
    { "yao", "要揺腰薬曜" },
    { "ye", "也野夜邪業葉" },
    { "yi", "一已亦依以移意医易伊役異億義議藝醫訳" },
    { "yin", "因引音飲銀" },
    { "ying", "英迎影映應營営" },
    { "yong", "永用泳擁" },
    { "you", "又有右友由尤油遊郵誘優" },
    { "yu", "予育余雨浴欲愈御宇域語於魚與込" },
    { "yuan", "元原源院員円園遠猿願" },
    { "yue", "月越約楽" },
    { "yun", "雲伝運" },
    { "za", "雑" },
    { "zai", "在再載災" },
    { "zang", "蔵" },
    { "zao", "早造" },
    { "ze", "則擇責" },
    { "zen", "怎" },
    { "zeng", "曾增増" },
    { "zha", "札" },
    { "zhai", "宅擇" },
    { "zhan", "站展戰戦" },
    { "zhang", "丈長障帳張" },
    { "zhao", "找着朝招" },
    { "zhe", "者這" },
    { "zhen", "真震針" },
    { "zheng", "正整争政爭" },
    { "zhi", "之只知支止制至治直指值置智値紙製質誌織隻識職執" },
    { "zhong", "中种終重種眾" },
    { "zhou", "周州昼宙洲週" },
    { "zhu", "助主住柱株祝逐注著諸屬術" },
    { "zhuan", "专專転" },
    { "zhuang", "状狀" },
    { "zhui", "追" },
    { "zhun", "準" },
    { "zhuo", "着" },
    { "zi", "子自字姉資" },
    { "zong", "總" },
    { "zuo", "左做昨坐座作" },
    { "zu", "足祖族卒組" },
    { "zui", "最酔" },
    { "zou", "走" },
    {NULL, NULL}
};
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_obj_t * lv_ime_pinyin_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_ime_pinyin_set_keyboard(lv_obj_t * obj, lv_obj_t * kb)
{
    if(kb) {
        LV_ASSERT_OBJ(kb, &lv_keyboard_class);
    }

    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(pinyin_ime->kb != NULL) {
        lv_obj_remove_event_cb_with_user_data(pinyin_ime->kb, lv_ime_pinyin_kb_event, obj);
    }
    pinyin_ime->kb = kb;
    if(kb == NULL) {
        lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_set_parent(obj, lv_obj_get_parent(kb));
    lv_obj_set_parent(pinyin_ime->cand_panel, lv_obj_get_parent(kb));
    lv_obj_add_event_cb(pinyin_ime->kb, lv_ime_pinyin_kb_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_align_to(pinyin_ime->cand_panel, pinyin_ime->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
    pinyin_update_keyboard_target(pinyin_ime);
    pinyin_update_panel_visibility(pinyin_ime);
}

void lv_ime_pinyin_set_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    init_pinyin_dict(obj, dict);
}

void lv_ime_pinyin_set_mode(lv_obj_t * obj, lv_ime_pinyin_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    LV_ASSERT_OBJ(pinyin_ime->kb, &lv_keyboard_class);

    pinyin_ime->mode = mode;

#if LV_IME_PINYIN_USE_K9_MODE
    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        pinyin_k9_init_data(obj);
        lv_keyboard_set_map(pinyin_ime->kb, LV_KEYBOARD_MODE_USER_1, (const char **)lv_btnm_def_pinyin_k9_map,
                            default_kb_ctrl_k9_map);
        lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_USER_1);
    }
    else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9_NUMBER) {
        lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_NUMBER);
    }
    else
#endif
    {
        lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    }
    pinyin_update_keyboard_target(pinyin_ime);
}

void lv_ime_pinyin_set_textarea(lv_obj_t * obj, lv_obj_t * textarea)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    if(textarea != NULL) LV_ASSERT_OBJ(textarea, &lv_textarea_class);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    pinyin_ime_clear_data(obj);
    pinyin_ime->textarea = textarea;
    pinyin_update_keyboard_target(pinyin_ime);
    pinyin_update_panel_visibility(pinyin_ime);
}

void lv_ime_pinyin_set_language(lv_obj_t * obj, lv_ime_pinyin_language_t language)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if((language != LV_IME_PINYIN_LANGUAGE_ENGLISH) &&
       (language != LV_IME_PINYIN_LANGUAGE_CHINESE)) return;

    pinyin_ime_clear_data(obj);
    pinyin_ime->language = language;
    pinyin_update_keyboard_target(pinyin_ime);
    pinyin_update_language_state(pinyin_ime);
    pinyin_update_panel_visibility(pinyin_ime);
}

void lv_ime_pinyin_reset(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    pinyin_ime_clear_data(obj);
    pinyin_update_panel_visibility((lv_ime_pinyin_t *)obj);
}

/*=====================
 * Getter functions
 *====================*/

lv_obj_t * lv_ime_pinyin_get_kb(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->kb;
}

lv_obj_t * lv_ime_pinyin_get_cand_panel(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->cand_panel;
}

lv_obj_t * lv_ime_pinyin_get_textarea(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_ime_pinyin_t *)obj)->textarea;
}

const lv_pinyin_dict_t * lv_ime_pinyin_get_dict(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->dict;
}

lv_ime_pinyin_language_t lv_ime_pinyin_get_language(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_ime_pinyin_t *)obj)->language;
}

/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t btnm_i;
    lv_btnm_def_pinyin_sel_map[IME_LANGUAGE_ID] = "\xE8\x8B\xB1";
    lv_btnm_def_pinyin_sel_map[IME_PAGE_PREV_ID] = "<";
    lv_btnm_def_pinyin_sel_map[IME_PREEDIT_ID] = pinyin_ime->preedit_display;
    for(btnm_i = 0; btnm_i < LV_IME_PINYIN_CAND_TEXT_NUM; btnm_i++) {
        lv_pinyin_cand_str[btnm_i][0] = ' ';
        lv_btnm_def_pinyin_sel_map[IME_CHINESE_FIRST_ID + btnm_i] = lv_pinyin_cand_str[btnm_i];
    }
    lv_btnm_def_pinyin_sel_map[IME_PAGE_NEXT_ID] = ">";
    lv_btnm_def_pinyin_sel_map[IME_PANEL_BUTTON_COUNT] = "";

    for(btnm_i = 0; btnm_i < IME_PANEL_BUTTON_COUNT; btnm_i++) {
        lv_btnm_def_pinyin_sel_ctrl[btnm_i] = LV_BUTTONMATRIX_CTRL_WIDTH_2;
    }
    lv_btnm_def_pinyin_sel_ctrl[IME_LANGUAGE_ID] = (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKABLE |
                                                        LV_BUTTONMATRIX_CTRL_WIDTH_2);
    lv_btnm_def_pinyin_sel_ctrl[IME_PAGE_PREV_ID] = LV_BUTTONMATRIX_CTRL_WIDTH_1;
    lv_btnm_def_pinyin_sel_ctrl[IME_PREEDIT_ID] = LV_BUTTONMATRIX_CTRL_WIDTH_4;
    lv_btnm_def_pinyin_sel_ctrl[IME_PAGE_NEXT_ID] = LV_BUTTONMATRIX_CTRL_WIDTH_1;

    pinyin_ime->mode = LV_IME_PINYIN_MODE_K26;
    pinyin_ime->language = LV_IME_PINYIN_LANGUAGE_ENGLISH;
    pinyin_ime->textarea = NULL;
    pinyin_ime->py_page = 0;
    pinyin_ime->ta_count = 0;
    pinyin_ime->cand_num = 0U;
    pinyin_ime->cand_str = NULL;
    pinyin_ime->py_page = 0U;
    pinyin_ime->cand_num = 0;
    lv_memzero(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
    lv_memzero(pinyin_ime->preedit, sizeof(pinyin_ime->preedit));
    lv_memzero(pinyin_ime->preedit_display, sizeof(pinyin_ime->preedit_display));
    pinyin_ime->preedit_display[0] = ' ';
    lv_memzero(pinyin_ime->py_num, sizeof(pinyin_ime->py_num));
    lv_memzero(pinyin_ime->py_pos, sizeof(pinyin_ime->py_pos));

    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

#if LV_IME_PINYIN_USE_DEFAULT_DICT
    init_pinyin_dict(obj, lv_ime_pinyin_def_dict);
#endif

    /* Init pinyin_ime->cand_panel */
    pinyin_ime->cand_panel = lv_buttonmatrix_create(lv_obj_get_parent(obj));
    lv_buttonmatrix_set_map(pinyin_ime->cand_panel, (const char **)lv_btnm_def_pinyin_sel_map);
    lv_buttonmatrix_set_ctrl_map(pinyin_ime->cand_panel, lv_btnm_def_pinyin_sel_ctrl);
    lv_obj_set_size(pinyin_ime->cand_panel, LV_PCT(100), 58);
    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);

    lv_buttonmatrix_set_one_checked(pinyin_ime->cand_panel, true);
    pinyin_update_language_state(pinyin_ime);
    lv_obj_remove_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* Set cand_panel style*/
    // Default style
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_0, 0);
    lv_obj_set_style_border_width(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_all(pinyin_ime->cand_panel, 8, 0);
    lv_obj_set_style_pad_gap(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_radius(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_gap(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_base_dir(pinyin_ime->cand_panel, LV_BASE_DIR_LTR, 0);

    // LV_PART_ITEMS style
    lv_obj_set_style_radius(pinyin_ime->cand_panel, 12, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(pinyin_ime->cand_panel, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(pinyin_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);

    // LV_PART_ITEMS | LV_STATE_PRESSED style
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(pinyin_ime->cand_panel, lv_color_white(), LV_PART_ITEMS | LV_STATE_PRESSED);

    /* event handler */
    lv_obj_add_event_cb(pinyin_ime->cand_panel, lv_ime_pinyin_cand_panel_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_add_event_cb(obj, lv_ime_pinyin_style_change_event, LV_EVENT_STYLE_CHANGED, NULL);

#if LV_IME_PINYIN_USE_K9_MODE
    pinyin_ime->k9_input_str_len = 0;
    pinyin_ime->k9_py_ll_pos = 0;
    pinyin_ime->k9_legal_py_count = 0;
    lv_memzero(pinyin_ime->k9_input_str, LV_IME_PINYIN_K9_MAX_INPUT);

    pinyin_k9_init_data(obj);

    lv_ll_init(&(pinyin_ime->k9_legal_py_ll), sizeof(ime_pinyin_k9_py_str_t));
#endif
}

static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(lv_obj_is_valid(pinyin_ime->kb))
        lv_obj_delete(pinyin_ime->kb);

    if(lv_obj_is_valid(pinyin_ime->cand_panel))
        lv_obj_delete(pinyin_ime->cand_panel);

#if LV_IME_PINYIN_USE_K9_MODE
    lv_ll_clear(&pinyin_ime->k9_legal_py_ll);
#endif
}

static void lv_ime_pinyin_kb_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_current_target(e);
    lv_obj_t * obj = lv_event_get_user_data(e);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    uint16_t btn_id;
    const char * txt;
    uint16_t len;

    if(code != LV_EVENT_VALUE_CHANGED) return;

    btn_id = lv_buttonmatrix_get_selected_button(kb);
    if(btn_id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    txt = lv_buttonmatrix_get_button_text(kb, btn_id);
    if((txt == NULL) || (pinyin_ime->textarea == NULL)) return;

    if(lv_strcmp(txt, LV_SYMBOL_KEYBOARD) == 0) {
        pinyin_ime_clear_data(obj);
        if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) {
            lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K9);
        }
        else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
            lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K26);
        }
        else {
            lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K9);
        }
        return;
    }

#if LV_IME_PINYIN_USE_K9_MODE
    if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) &&
       (lv_strcmp(txt, "123") == 0)) {
        pinyin_ime_clear_data(obj);
        lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K9_NUMBER);
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
        return;
    }

    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9_NUMBER) return;

    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        static const char * k9_py_map[8] = {
            "abc", "def", "ghi", "jkl", "mno", "pqrs", "tuv", "wxyz"
        };
        uint16_t group;

        if((btn_id >= 16U) &&
           (btn_id < (16U + LV_IME_PINYIN_K9_CAND_TEXT_NUM)) &&
           (pinyin_ime->ta_count > 0U) && (txt[0] != ' ')) {
            lv_strlcpy(pinyin_ime->preedit, txt, sizeof(pinyin_ime->preedit));
            if(pinyin_ime->language == LV_IME_PINYIN_LANGUAGE_CHINESE) {
                pinyin_input_proc(obj);
            }
            else {
                lv_textarea_add_text(pinyin_ime->textarea, pinyin_ime->preedit);
                pinyin_ime_clear_data(obj);
            }
            return;
        }

        if(lv_strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            if(pinyin_ime->ta_count > 0U) {
                pinyin_ime->ta_count--;
                pinyin_ime->k9_input_str[pinyin_ime->ta_count] = '\0';
                lv_ll_clear(&pinyin_ime->k9_legal_py_ll);
                pinyin_ime->k9_legal_py_count = 0U;
                if(pinyin_ime->ta_count > 0U) {
                    pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
                    if(pinyin_ime->k9_legal_py_count > 0U) pinyin_k9_fill_cand(obj);
                }
                else {
                    pinyin_ime_clear_data(obj);
                }
            }
            else {
                lv_textarea_delete_char(pinyin_ime->textarea);
            }
            return;
        }

        if(lv_strcmp(txt, LV_SYMBOL_LEFT) == 0) {
            pinyin_k9_cand_page_proc(obj, 0U);
            return;
        }
        if(lv_strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
            pinyin_k9_cand_page_proc(obj, 1U);
            return;
        }

        for(group = 0U; group < 8U; group++) {
            if((lv_strcmp(txt, k9_py_map[group]) == 0) ||
               ((group == 0U) && (lv_strcmp(txt, "abc ") == 0))) {
                uint16_t old_count = pinyin_ime->ta_count;
                if(old_count >= (LV_IME_PINYIN_K9_MAX_INPUT - 1U)) return;
                pinyin_ime->k9_input_str[old_count] = (char)('2' + group);
                pinyin_ime->k9_input_str[old_count + 1U] = '\0';
                lv_ll_clear(&pinyin_ime->k9_legal_py_ll);
                pinyin_ime->k9_legal_py_count = 0U;
                pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
                if(pinyin_ime->k9_legal_py_count > 0U) {
                    pinyin_k9_fill_cand(obj);
                }
                else {
                    pinyin_ime->k9_input_str[old_count] = '\0';
                    pinyin_ime->ta_count = old_count;
                }
                return;
            }
        }

        if((lv_strcmp(txt, "Enter") == 0) ||
           (lv_strcmp(txt, LV_SYMBOL_NEW_LINE) == 0)) {
            pinyin_ime_clear_data(obj);
            lv_textarea_add_char(pinyin_ime->textarea, '\n');
        }
        return;
    }
#endif

    if(pinyin_ime->language == LV_IME_PINYIN_LANGUAGE_ENGLISH) return;

    len = lv_strlen(pinyin_ime->preedit);
    if(((txt[0] >= 'a') && (txt[0] <= 'z') && (txt[1] == '\0')) ||
       ((txt[0] >= 'A') && (txt[0] <= 'Z') && (txt[1] == '\0'))) {
        if(len < LV_IME_PINYIN_PREEDIT_MAX) {
            pinyin_ime->preedit[len] = txt[0];
            pinyin_ime->preedit[len + 1U] = '\0';
            pinyin_input_proc(obj);
        }
        return;
    }

    if(lv_strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        if(len > 0U) {
            pinyin_ime->preedit[len - 1U] = '\0';
            if(len == 1U) pinyin_ime_clear_data(obj);
            else pinyin_input_proc(obj);
        }
        return;
    }

    if((lv_strcmp(txt, "ABC") == 0) || (lv_strcmp(txt, "abc") == 0)) {
        return;
    }

    if(len > 0U) {
        return;
    }

    if(lv_strcmp(txt, LV_SYMBOL_LEFT) == 0) {
        uint32_t pos = lv_textarea_get_cursor_pos(pinyin_ime->textarea);
        if(pos > 0U) lv_textarea_set_cursor_pos(pinyin_ime->textarea, (int32_t)pos - 1);
    }
    else if(lv_strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
        uint32_t pos = lv_textarea_get_cursor_pos(pinyin_ime->textarea);
        lv_textarea_set_cursor_pos(pinyin_ime->textarea, (int32_t)pos + 1);
    }
    else if((lv_strcmp(txt, "1#") != 0) &&
            (lv_strcmp(txt, LV_SYMBOL_OK) != 0) &&
            (lv_strcmp(txt, "Enter") != 0)) {
        lv_textarea_add_text(pinyin_ime->textarea, txt);
    }
}

static void lv_ime_pinyin_cand_panel_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cand_panel = lv_event_get_current_target(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_buttonmatrix_get_selected_button(cand_panel);
        const char * txt;

        if(id == LV_BUTTONMATRIX_BUTTON_NONE) return;
        if(id == IME_LANGUAGE_ID) {
            lv_ime_pinyin_set_language(obj,
                pinyin_ime->language == LV_IME_PINYIN_LANGUAGE_ENGLISH ?
                LV_IME_PINYIN_LANGUAGE_CHINESE :
                LV_IME_PINYIN_LANGUAGE_ENGLISH);
            return;
        }
        if(id == IME_PAGE_PREV_ID) {
            pinyin_page_proc(obj, 0);
            return;
        }
        if(id == IME_PAGE_NEXT_ID) {
            pinyin_page_proc(obj, 1);
            return;
        }
        if((pinyin_ime->textarea == NULL) || (pinyin_ime->preedit[0] == '\0')) return;

        if(id == IME_PREEDIT_ID) txt = pinyin_ime->preedit;
        else if((id >= IME_CHINESE_FIRST_ID) && (id < IME_PAGE_NEXT_ID))
            txt = lv_buttonmatrix_get_button_text(cand_panel, id);
        else return;

        if((txt == NULL) || (txt[0] == '\0') || ((txt[0] == ' ') && (txt[1] == '\0'))) return;
        lv_textarea_add_text(pinyin_ime->textarea, txt);
        pinyin_ime_clear_data(obj);
        pinyin_update_panel_visibility(pinyin_ime);
    }
}

static void pinyin_input_proc(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    uint16_t raw_len = lv_strlen(pinyin_ime->preedit);
    uint16_t i;

    lv_memzero(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
    pinyin_ime->cand_str = NULL;
    pinyin_ime->cand_num = 0U;
    pinyin_ime->py_page = 0U;

    if((raw_len > 0U) && (raw_len < sizeof(pinyin_ime->input_char))) {
        for(i = 0U; i < raw_len; i++) {
            char c = pinyin_ime->preedit[i];
            pinyin_ime->input_char[i] = ((c >= 'A') && (c <= 'Z')) ?
                                            (char)(c - 'A' + 'a') : c;
        }
        pinyin_ime->input_char[raw_len] = '\0';
        pinyin_ime->cand_str = pinyin_search_matching(
            obj, pinyin_ime->input_char, &pinyin_ime->cand_num);
    }

    pinyin_update_preedit_display(pinyin_ime);
    pinyin_fill_candidate_page(pinyin_ime);
    pinyin_update_panel_visibility(pinyin_ime);
    lv_obj_invalidate(pinyin_ime->cand_panel);
}

static void pinyin_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    uint16_t page_num;

    if((pinyin_ime->cand_str == NULL) || (pinyin_ime->cand_num == 0U)) return;
    page_num = (uint16_t)((pinyin_ime->cand_num - 1U) / LV_IME_PINYIN_CAND_TEXT_NUM);

    if(dir == 0) {
        if(pinyin_ime->py_page == 0U) return;
        pinyin_ime->py_page--;
    }
    else {
        if(pinyin_ime->py_page >= page_num) return;
        pinyin_ime->py_page++;
    }

    pinyin_fill_candidate_page(pinyin_ime);
    lv_obj_invalidate(pinyin_ime->cand_panel);
}

static void pinyin_update_preedit_display(lv_ime_pinyin_t * pinyin_ime)
{
    uint16_t len = lv_strlen(pinyin_ime->preedit);
    lv_memzero(pinyin_ime->preedit_display, sizeof(pinyin_ime->preedit_display));
    if(len == 0U) {
        pinyin_ime->preedit_display[0] = ' ';
    }
    else if(len < sizeof(pinyin_ime->preedit_display)) {
        lv_strlcpy(pinyin_ime->preedit_display, pinyin_ime->preedit,
                   sizeof(pinyin_ime->preedit_display));
    }
    else {
        lv_memcpy(pinyin_ime->preedit_display, pinyin_ime->preedit, 10U);
        lv_memcpy(&pinyin_ime->preedit_display[10], "...", 3U);
        lv_memcpy(&pinyin_ime->preedit_display[13],
                  &pinyin_ime->preedit[len - 9U], 9U);
        pinyin_ime->preedit_display[22] = '\0';
    }
}

static void pinyin_fill_candidate_page(lv_ime_pinyin_t * pinyin_ime)
{
    uint16_t i;
    uint16_t offset = pinyin_ime->py_page * LV_IME_PINYIN_CAND_TEXT_NUM;

    for(i = 0U; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        lv_memzero(lv_pinyin_cand_str[i], sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
        if((pinyin_ime->cand_str != NULL) && ((offset + i) < pinyin_ime->cand_num)) {
            lv_memcpy(lv_pinyin_cand_str[i],
                      &pinyin_ime->cand_str[(offset + i) * 3U], 3U);
            lv_pinyin_cand_str[i][3] = '\0';
        }
    }
}

static void pinyin_update_language_state(lv_ime_pinyin_t * pinyin_ime)
{
    lv_btnm_def_pinyin_sel_map[IME_LANGUAGE_ID] =
        pinyin_ime->language == LV_IME_PINYIN_LANGUAGE_CHINESE ?
        "\xE4\xB8\xAD" : "\xE8\x8B\xB1";
    lv_buttonmatrix_set_button_ctrl(pinyin_ime->cand_panel,
        IME_LANGUAGE_ID, LV_BUTTONMATRIX_CTRL_CHECKED);
    lv_obj_invalidate(pinyin_ime->cand_panel);
}

static void pinyin_update_panel_visibility(lv_ime_pinyin_t * pinyin_ime)
{
    if((pinyin_ime->textarea != NULL) && (pinyin_ime->kb != NULL) &&
       !lv_obj_has_flag(pinyin_ime->kb, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_remove_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void pinyin_update_keyboard_target(lv_ime_pinyin_t * pinyin_ime)
{
    bool direct_input;

    if(pinyin_ime->kb == NULL) return;
    direct_input = (pinyin_ime->mode == LV_IME_PINYIN_MODE_K9_NUMBER) ||
                   ((pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) &&
                    (pinyin_ime->language == LV_IME_PINYIN_LANGUAGE_ENGLISH));
    lv_keyboard_set_textarea(pinyin_ime->kb,
                             direct_input ? pinyin_ime->textarea : NULL);
}

static void lv_ime_pinyin_style_change_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_STYLE_CHANGED) {
        const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_obj_set_style_text_font(pinyin_ime->cand_panel, font, 0);
    }
}

static void init_pinyin_dict(lv_obj_t * obj, const lv_pinyin_dict_t * dict)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    char headletter = 'a';
    uint16_t offset_sum = 0;
    uint16_t offset_count = 0;
    uint16_t letter_calc = 0;

    pinyin_ime->dict = dict;

    for(uint16_t i = 0; ; i++) {
        if((NULL == (dict[i].py)) || (NULL == (dict[i].py_mb))) {
            headletter = dict[i - 1].py[0];
            letter_calc = headletter - 'a';
            pinyin_ime->py_num[letter_calc] = offset_count;
            break;
        }

        if(headletter == (dict[i].py[0])) {
            offset_count++;
        }
        else {
            headletter = dict[i].py[0];
            pinyin_ime->py_num[letter_calc] = offset_count;
            letter_calc = headletter - 'a';
            offset_sum += offset_count;
            pinyin_ime->py_pos[letter_calc] = offset_sum;

            offset_count = 1;
        }
    }
}

static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    const lv_pinyin_dict_t * cpHZ;
    uint8_t index, len = 0, offset;
    volatile uint8_t count = 0;

    if(*py_str == '\0')    return NULL;
    if(*py_str == 'i')     return NULL;
    if(*py_str == 'u')     return NULL;
    if(*py_str == 'v')     return NULL;
    if(*py_str == ' ')     return NULL;

    offset = py_str[0] - 'a';
    len = lv_strlen(py_str);

    cpHZ  = &pinyin_ime->dict[pinyin_ime->py_pos[offset]];
    count = pinyin_ime->py_num[offset];

    while(count--) {
        for(index = 0; index < len; index++) {
            if(*(py_str + index) != *((cpHZ->py) + index)) {
                break;
            }
        }

        // perfect match
        if(len == 1 || index == len) {
            // The Chinese character in UTF-8 encoding format is 3 bytes
            * cand_num = lv_strlen((const char *)(cpHZ->py_mb)) / 3;
            return (char *)(cpHZ->py_mb);
        }
        cpHZ++;
    }
    return NULL;
}

static void pinyin_ime_clear_data(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

#if LV_IME_PINYIN_USE_K9_MODE
    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        pinyin_ime->k9_input_str_len = 0;
        pinyin_ime->k9_py_ll_pos = 0;
        pinyin_ime->k9_legal_py_count = 0;
        lv_memzero(pinyin_ime->k9_input_str,  LV_IME_PINYIN_K9_MAX_INPUT);
        lv_memzero(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        for(uint8_t i = 0; i < LV_IME_PINYIN_K9_CAND_TEXT_NUM; i++) {
            lv_strlcpy(lv_pinyin_k9_cand_str[i], " ", LV_IME_PINYIN_K9_MAX_INPUT);
        }
        lv_strlcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0", LV_IME_PINYIN_K9_MAX_INPUT);
        lv_strlcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0", LV_IME_PINYIN_K9_MAX_INPUT);
        lv_ll_clear(&pinyin_ime->k9_legal_py_ll);
        lv_buttonmatrix_set_map(pinyin_ime->kb, (const char **)lv_btnm_def_pinyin_k9_map);
    }
#endif

    pinyin_ime->ta_count = 0;
    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        lv_memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }
    lv_memzero(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
    lv_memzero(pinyin_ime->preedit, sizeof(pinyin_ime->preedit));
    lv_memzero(pinyin_ime->preedit_display, sizeof(pinyin_ime->preedit_display));
    pinyin_ime->preedit_display[0] = ' ';
    lv_obj_invalidate(pinyin_ime->cand_panel);
}

#if LV_IME_PINYIN_USE_K9_MODE
static void pinyin_k9_init_data(lv_obj_t * obj)
{
    LV_UNUSED(obj);

    uint16_t py_str_i = 0;
    uint16_t btnm_i = 0;
    for(btnm_i = 19; btnm_i < (LV_IME_PINYIN_K9_CAND_TEXT_NUM + 21); btnm_i++) {
        if(py_str_i == LV_IME_PINYIN_K9_CAND_TEXT_NUM) {
            lv_strlcpy(lv_pinyin_k9_cand_str[py_str_i], LV_SYMBOL_RIGHT"\0", LV_IME_PINYIN_K9_MAX_INPUT);
        }
        else if(py_str_i == LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1) {
            lv_strlcpy(lv_pinyin_k9_cand_str[py_str_i], "\0", LV_IME_PINYIN_K9_MAX_INPUT);
        }
        else {
            lv_strlcpy(lv_pinyin_k9_cand_str[py_str_i], " \0", LV_IME_PINYIN_K9_MAX_INPUT);
        }

        lv_btnm_def_pinyin_k9_map[btnm_i] = lv_pinyin_k9_cand_str[py_str_i];
        py_str_i++;
    }

    default_kb_ctrl_k9_map[0]  = (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED |
                                      LV_BUTTONMATRIX_CTRL_WIDTH_1);
    default_kb_ctrl_k9_map[1]  = (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_NO_REPEAT |
                                      LV_BUTTONMATRIX_CTRL_CLICK_TRIG | LV_BUTTONMATRIX_CTRL_WIDTH_1);
    default_kb_ctrl_k9_map[4]  = (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED |
                                      LV_BUTTONMATRIX_CTRL_WIDTH_1);
    default_kb_ctrl_k9_map[5]  = (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED |
                                      LV_BUTTONMATRIX_CTRL_WIDTH_1);
    default_kb_ctrl_k9_map[9]  = (lv_buttonmatrix_ctrl_t)(LV_KEYBOARD_CTRL_BUTTON_FLAGS |
                                      LV_BUTTONMATRIX_CTRL_WIDTH_1);
    default_kb_ctrl_k9_map[10] = (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED |
                                      LV_BUTTONMATRIX_CTRL_WIDTH_1);
    default_kb_ctrl_k9_map[14] = (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED |
                                      LV_BUTTONMATRIX_CTRL_WIDTH_1);
    default_kb_ctrl_k9_map[15] = (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED |
                                      LV_BUTTONMATRIX_CTRL_WIDTH_1);
    default_kb_ctrl_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 16] =
        (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_CHECKED | LV_BUTTONMATRIX_CTRL_WIDTH_1);
}

static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[])
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t len = lv_strlen(k9_input);

    if((len == 0) || (len >= LV_IME_PINYIN_K9_MAX_INPUT)) {
        return;
    }

    char py_comp[LV_IME_PINYIN_K9_MAX_INPUT] = {0};
    int mark[LV_IME_PINYIN_K9_MAX_INPUT] = {0};
    int index = 0;
    int flag = 0;
    uint16_t count = 0;

    uint32_t ll_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    ll_len = lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);
    ll_index = lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);

    while(index != -1) {
        if(index == len) {
            if(pinyin_k9_is_valid_py(obj, py_comp)) {
                if((count >= ll_len) || (ll_len == 0)) {
                    ll_index = lv_ll_ins_tail(&pinyin_ime->k9_legal_py_ll);
                    lv_strlcpy(ll_index->py_str, py_comp, sizeof(ll_index->py_str));
                }
                else if((count < ll_len)) {
                    lv_strlcpy(ll_index->py_str, py_comp, sizeof(ll_index->py_str));
                    ll_index = lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
                }
                count++;
            }
            index--;
        }
        else {
            flag = mark[index];
            if((size_t)flag < lv_strlen(py9_map[k9_input[index] - '2'])) {
                py_comp[index] = py9_map[k9_input[index] - '2'][flag];
                mark[index] = mark[index] + 1;
                index++;
            }
            else {
                mark[index] = 0;
                index--;
            }
        }
    }

    if(count > 0) {
        pinyin_ime->ta_count = len;
        pinyin_ime->k9_legal_py_count = count;
    }
}

/*true: visible; false: not visible*/
static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    const lv_pinyin_dict_t * cpHZ = NULL;
    uint8_t index = 0, len = 0, offset = 0;
    volatile uint8_t count = 0;

    if(*py_str == '\0')    return false;
    if(*py_str == 'i')     return false;
    if(*py_str == 'u')     return false;
    if(*py_str == 'v')     return false;

    offset = py_str[0] - 'a';
    len = lv_strlen(py_str);

    cpHZ  = &pinyin_ime->dict[pinyin_ime->py_pos[offset]];
    count = pinyin_ime->py_num[offset];

    while(count--) {
        for(index = 0; index < len; index++) {
            if(*(py_str + index) != *((cpHZ->py) + index)) {
                break;
            }
        }

        // perfect match
        if(len == 1 || index == len) {
            return true;
        }
        cpHZ++;
    }
    return false;
}

static void pinyin_k9_fill_cand(lv_obj_t * obj)
{
    uint16_t index = 0, tmp_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    tmp_len = pinyin_ime->k9_legal_py_count;

    if(tmp_len != cand_len) {
        lv_memzero(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        lv_strlcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0", LV_IME_PINYIN_K9_MAX_INPUT);
        lv_strlcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0", LV_IME_PINYIN_K9_MAX_INPUT);
        cand_len = tmp_len;
    }

    ll_index = lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
    lv_strlcpy(pinyin_ime->input_char, ll_index->py_str, sizeof(pinyin_ime->input_char));

    for(uint8_t i = 0; i < LV_IME_PINYIN_K9_CAND_TEXT_NUM; i++) {
        lv_strlcpy(lv_pinyin_k9_cand_str[i], " ", LV_IME_PINYIN_K9_MAX_INPUT);
    }

    while(ll_index) {
        if(index >= LV_IME_PINYIN_K9_CAND_TEXT_NUM)
            break;

        if(index < pinyin_ime->k9_legal_py_count) {
            lv_strlcpy(lv_pinyin_k9_cand_str[index], ll_index->py_str, LV_IME_PINYIN_K9_MAX_INPUT);
        }

        ll_index = lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
        index++;
    }
    pinyin_ime->k9_py_ll_pos = index;

    pinyin_ime->k9_input_str_len = lv_strlen(pinyin_ime->input_char);
}

static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t ll_len =  lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);

    if((ll_len > LV_IME_PINYIN_K9_CAND_TEXT_NUM) && (pinyin_ime->k9_legal_py_count > LV_IME_PINYIN_K9_CAND_TEXT_NUM)) {
        ime_pinyin_k9_py_str_t * ll_index = NULL;
        int count = 0;

        ll_index = lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
        while(ll_index) {
            if(count >= pinyin_ime->k9_py_ll_pos)   break;

            ll_index = lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
            count++;
        }

        if((NULL == ll_index) && (dir == 1))   return;

        lv_memzero(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        lv_strlcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0", LV_IME_PINYIN_K9_MAX_INPUT);
        lv_strlcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0", LV_IME_PINYIN_K9_MAX_INPUT);

        // next page
        if(dir == 1) {
            for(uint8_t i = 0; i < LV_IME_PINYIN_K9_CAND_TEXT_NUM; i++) {
                lv_strlcpy(lv_pinyin_k9_cand_str[i], " ", LV_IME_PINYIN_K9_MAX_INPUT);
            }

            count = 0;
            while(ll_index) {
                if(count >= (LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1))
                    break;

                lv_strlcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str, LV_IME_PINYIN_K9_MAX_INPUT);
                ll_index = lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
                count++;
            }
            pinyin_ime->k9_py_ll_pos += count - 1;

        }
        // previous page
        else {
            for(uint8_t i = 0; i < LV_IME_PINYIN_K9_CAND_TEXT_NUM; i++) {
                lv_strlcpy(lv_pinyin_k9_cand_str[i], " ", LV_IME_PINYIN_K9_MAX_INPUT);
            }
            count = LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1;
            ll_index = lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index);
            while(ll_index) {
                if(count < 0)  break;

                lv_strlcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str, LV_IME_PINYIN_K9_MAX_INPUT);
                ll_index = lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the previous list*/
                count--;
            }

            if(pinyin_ime->k9_py_ll_pos > LV_IME_PINYIN_K9_CAND_TEXT_NUM)
                pinyin_ime->k9_py_ll_pos -= 1;
        }

    }
}

#endif  /*LV_IME_PINYIN_USE_K9_MODE*/

#endif  /*LV_USE_IME_PINYIN*/
