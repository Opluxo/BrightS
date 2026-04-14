#include "im.h"
#include "fb.h"
#include "font.h"
#include "tty.h"
#include "serial.h"

#define BRIGHTS_COM1_PORT 0x3F8

static int im_active = 0;
static char pinyin_buf[IM_MAX_PINYIN];
static int pinyin_len = 0;
static char candidates[IM_MAX_CANDIDATES][16];
static int candidate_count = 0;
static int selected_candidate = 0;

static const char *common_chars[] = {
    "的", "一", "是", "了", "在", "人", "有", "我", "他", "这",
    "中", "大", "来", "上", "国", "个", "到", "说", "们", "为",
    "子", "和", "你", "地", "道", "出", "而", "于", "去", "过",
    "家", "会", "也", "对", "生", "能", "而", "着", "下", "分",
    "还", "起", "就", "年", "时", "候", "看", "发", "后", "作",
    "里", "用", "道", "然", "家", "那", "得", "着", "方", "多",
    "好", "小", "部", "其", "些", "现", "当", "正", "定", "见",
    "只", "主", "没", "从", "动", "身", "全", "然", "心", "事",
    "工", "每", "实", "本", "解", "此", "制", "应", "先", "头",
    "己", "知", "她", "因", "但", "只", "被", "让", "或", "从"
};

static const char *pinyin_map[][10] = {
    {"de", "的", "得", "地", "德", "嘚", "", "", "", ""},
    {"yi", "一", "以", "意", "义", "已", "医", "衣", "益", "易"},
    {"shi", "是", "时", "十", "事", "实", "市", "使", "世", "式"},
    {"le", "了", "乐", "勒", "雷", "肋", "累", "", "", ""},
    {"zai", "在", "再", "载", "仔", "灾", "", "", "", ""},
    {"ren", "人", "仁", "忍", "认", "任", "壬", "刃", "韧", ""},
    {"you", "有", "又", "由", "友", "右", "优", "油", "游", "幼"},
    {"wo", "我", "握", "卧", "沃", "", "", "", "", ""},
    {"ta", "他", "她", "它", "塔", "踏", "獭", "", "", ""},
    {"zhe", "这", "者", "着", "折", "哲", "浙", "", "", ""},
    {"zhong", "中", "种", "重", "众", "终", "钟", "忠", "肿", ""},
    {"da", "大", "打", "达", "答", "大", "沓", "", "", ""},
    {"lai", "来", "赖", "莱", "睐", "籁", "", "", "", ""},
    {"shang", "上", "尚", "商", "伤", "裳", "", "", "", ""},
    {"guo", "国", "过", "果", "郭", "锅", "裹", "", "", ""},
    {"ge", "个", "各", "歌", "哥", "革", "格", "隔", "合", ""},
    {"dao", "到", "道", "导", "倒", "岛", "稻", "悼", "蹈", ""},
    {"shuo", "说", "硕", "朔", "烁", "铄", "", "", "", ""},
    {"men", "们", "门", "闷", "们", "焖", "", "", "", ""},
    {"wei", "为", "位", "未", "围", "维", "微", "伟", "委", "卫"},
    {"zi", "子", "字", "自", "资", "紫", "兹", "孜", "咨", "吱"},
    {"he", "和", "合", "河", "何", "荷", "核", "喝", "贺", "盒"},
    {"ni", "你", "尼", "泥", "呢", "拟", "腻", "", "", ""},
    {"di", "地", "的", "第", "低", "底", "递", "敌", "笛", "蒂"},
    {"chu", "出", "处", "初", "除", "楚", "储", "厨", "雏", ""},
    {"er", "而", "二", "尔", "耳", "儿", "贰", "", "", ""},
    {"yu", "于", "与", "预", "域", "欲", "遇", "喻", "御", "裕"},
    {"qu", "去", "取", "区", "曲", "趋", "渠", "趣", "躯", ""},
    {"guo2", "过", "国", "果", "郭", "锅", "", "", "", ""},
    {"jia", "家", "加", "假", "价", "架", "嘉", "夹", "驾", "嫁"},
    {"hui", "会", "回", "汇", "绘", "悔", "毁", "惠", "辉", "慧"},
    {"ye", "也", "业", "夜", "叶", "野", "液", "页", "爷", "椰"},
    {"dui", "对", "队", "堆", "兑", "怼", "碓", "", "", ""},
    {"sheng", "生", "声", "升", "胜", "省", "圣", "盛", "牲", "甥"},
    {"neng", "能", "而", "", "", "", "", "", "", ""},
    {"zhe2", "着", "这", "者", "浙", "", "", "", "", ""},
    {"xia", "下", "夏", "吓", "侠", "峡", "狭", "霞", "暇", "虾"},
    {"fen", "分", "份", "粉", "纷", "奋", "粪", "愤", "氛", "坟"},
    {"hai", "还", "海", "孩", "害", "亥", "骇", "骸", "", ""},
    {"qi", "起", "其", "气", "七", "企", "器", "汽", "奇", "齐"},
    {"jiu", "就", "九", "久", "酒", "旧", "救", "究", "舅", "韭"},
    {"shi2", "时", "十", "事", "实", "市", "使", "世", "是", "式"},
    {"hou", "后", "候", "厚", "侯", "忽", "乎", "胡", "湖", "互"},
    {"kan", "看", "刊", "坎", "砍", "堪", "勘", "侃", "", ""},
    {"fa", "发", "法", "乏", "伐", "阀", "筏", "", "", ""},
    {"hou2", "后", "候", "厚", "侯", "忽", "", "", "", ""},
    {"zuo", "作", "做", "坐", "座", "左", "佐", "琢", "奏", "揍"},
    {"li", "里", "力", "理", "利", "立", "李", "历", "离", "礼"},
    {"yong", "用", "永", "勇", "涌", "泳", "拥", "壅", "", ""},
    {"ran", "然", "燃", "染", "冉", "苒", "", "", "", ""},
    {"na", "那", "哪", "纳", "娜", "拿", "呐", "钠", "南", ""},
    {"de2", "的", "得", "地", "", "", "", "", "", ""},
    {"hao", "好", "号", "浩", "皓", "豪", "毫", "郝", "耗", "昊"},
    {"xiao", "小", "笑", "效", "校", "消", "晓", "萧", "孝", "肖"},
    {"bu", "不", "部", "步", "布", "补", "簿", "堡", "怖", ""},
    {"qi2", "其", "期", "七", "企", "起", "气", "奇", "器", "齐"},
    {"xian", "现", "先", "县", "线", "显", "险", "限", "宪", "献"},
    {"dang", "当", "党", "档", "荡", "挡", "刀", "叨", "蹈", ""},
    {"zheng", "正", "政", "证", "争", "整", "郑", "征", "帧", "蒸"},
    {"jian", "见", "间", "建", "件", "简", "键", "监", "减", "检"},
    {"zhi", "只", "知", "之", "至", "治", "制", "质", "指", "纸"},
    {"zhu", "主", "注", "住", "助", "逐", "著", "驻", "煮", "柱"},
    {"mei", "没", "每", "美", "妹", "眉", "梅", "媒", "煤", "枚"},
    {"cong", "从", "此", "次", "聪", "丛", "匆", "葱", "", ""},
    {"dong", "动", "东", "冬", "懂", "洞", "冻", "栋", "董", "咚"},
    {"shen", "身", "深", "什", "神", "审", "甚", "肾", "沈", "渗"},
    {"quan", "全", "权", "泉", "圈", "劝", "拳", "犬", "券", "颧"},
    {"xin", "心", "新", "信", "辛", "薪", "欣", "馨", "鑫", "昕"},
    {"shi3", "事", "是", "时", "十", "实", "市", "使", "世", "式"},
    {"gong", "工", "公", "共", "功", "供", "攻", "宫", "恭", "躬"},
    {"mei2", "每", "没", "美", "妹", "眉", "梅", "媒", "煤", "枚"},
    {"ben", "本", "奔", "苯", "笨", "夯", "", "", "", ""},
    {"jie", "解", "结", "接", "姐", "街", "节", "洁", "截", "届"},
    {"ci", "此", "次", "从", "词", "辞", "慈", "磁", "刺", "瓷"},
    {"zhi2", "制", "治", "致", "质", "志", "置", "智", "直", "值"},
    {"ying", "应", "英", "影", "营", "映", "硬", "赢", "迎", "颖"},
    {"xian2", "先", "现", "县", "线", "显", "险", "限", "宪", "献"},
    {"tou", "头", "投", "透", "偷", "凸", "秃", "", "", ""},
    {"ji", "己", "机", "记", "济", "技", "集", "极", "击", "积"},
    {"zhi3", "知", "只", "之", "至", "治", "制", "质", "指", "纸"},
    {"ta2", "她", "他", "它", "塔", "踏", "獭", "", "", ""},
    {"yin", "因", "银", "引", "印", "音", "阴", "饮", "隐", "寅"},
    {"dan", "但", "单", "蛋", "淡", "胆", "弹", "丹", "担", "氮"},
    {"zhi4", "只", "知", "之", "至", "治", "制", "质", "指", "纸"},
    {"bei", "被", "北", "备", "背", "倍", "杯", "贝", "辈", "悲"},
    {"rang", "让", "嚷", "壤", "攘", "瓢", "", "", "", ""},
    {"huo", "或", "活", "火", "伙", "获", "祸", "霍", "货", "惑"},
    {"cong2", "从", "此", "次", "聪", "丛", "匆", "葱", "", ""}
};

static const int pinyin_map_size = sizeof(pinyin_map) / sizeof(pinyin_map[0]);

static void search_pinyin(const char *pinyin)
{
    candidate_count = 0;
    selected_candidate = 0;
    
    if (pinyin_len == 0) {
        return;
    }
    
    for (int i = 0; i < pinyin_map_size && candidate_count < IM_MAX_CANDIDATES; i++) {
        if (pinyin_map[i][0] && pinyin_map[i][0][0] == pinyin[0]) {
            int match = 1;
            for (int j = 0; j < pinyin_len && pinyin_map[i][0][j]; j++) {
                if (pinyin_map[i][0][j] != pinyin[j]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                for (int j = 1; j < 10 && pinyin_map[i][j][0] && candidate_count < IM_MAX_CANDIDATES; j++) {
                    int len = 0;
                    while (pinyin_map[i][j][len] && len < 15) {
                        candidates[candidate_count][len] = pinyin_map[i][j][len];
                        len++;
                    }
                    candidates[candidate_count][len] = 0;
                    candidate_count++;
                }
            }
        }
    }
    
    if (candidate_count == 0 && pinyin_len >= 2) {
        for (int i = 0; i < 50 && candidate_count < IM_MAX_CANDIDATES; i++) {
            int len = 0;
            while (common_chars[i][len]) {
                candidates[candidate_count][len] = common_chars[i][len];
                len++;
            }
            candidates[candidate_count][len] = 0;
            candidate_count++;
        }
    }
}

void brights_im_init(void)
{
    im_active = 0;
    pinyin_len = 0;
    candidate_count = 0;
    selected_candidate = 0;
}

int brights_im_is_active(void)
{
    return im_active;
}

void brights_im_toggle(void)
{
    im_active = !im_active;
    if (!im_active) {
        brights_im_clear();
    }
}

void brights_im_handle_char(char ch)
{
    if (!im_active) return;
    
    if (ch >= 'a' && ch <= 'z') {
        if (pinyin_len < IM_MAX_PINYIN - 1) {
            pinyin_buf[pinyin_len++] = ch;
            pinyin_buf[pinyin_len] = 0;
            search_pinyin(pinyin_buf);
        }
    } else if (ch == '\b') {
        brights_im_backspace();
    } else if (ch == '\n' || ch == '\r') {
        if (candidate_count > 0) {
            brights_im_select_candidate(selected_candidate);
        } else {
            brights_im_commit();
        }
    }
}

void brights_im_handle_special(int key)
{
    if (!im_active) return;
    
    if (key == 1) {
        selected_candidate = (selected_candidate + 1) % candidate_count;
    } else if (key == 2) {
        selected_candidate = (selected_candidate - 1 + candidate_count) % candidate_count;
    } else if (key >= '1' && key <= '9' && key - '1' < candidate_count) {
        brights_im_select_candidate(key - '1');
    } else if (key == '0' && 9 < candidate_count) {
        brights_im_select_candidate(9);
    }
}

const char *brights_im_get_preedit(void)
{
    return pinyin_buf;
}

int brights_im_get_candidate_count(void)
{
    return candidate_count;
}

const char *brights_im_get_candidate(int index)
{
    if (index < 0 || index >= candidate_count) {
        return "";
    }
    return candidates[index];
}

void brights_im_select_candidate(int index)
{
    if (index < 0 || index >= candidate_count) return;
    
    brights_tty_write_str(candidates[index]);
    brights_im_clear();
}

void brights_im_commit(void)
{
    for (int i = 0; i < pinyin_len; i++) {
        char s[2] = {pinyin_buf[i], 0};
        brights_tty_write_str(s);
    }
    brights_im_clear();
}

void brights_im_backspace(void)
{
    if (pinyin_len > 0) {
        pinyin_len--;
        pinyin_buf[pinyin_len] = 0;
        if (pinyin_len > 0) {
            search_pinyin(pinyin_buf);
        } else {
            candidate_count = 0;
        }
    }
}

void brights_im_clear(void)
{
    pinyin_len = 0;
    pinyin_buf[0] = 0;
    candidate_count = 0;
    selected_candidate = 0;
}

int brights_im_cursor_pos(void)
{
    return pinyin_len;
}

void brights_im_draw_candidates(void)
{
    if (!brights_fb_available()) return;
    
    brights_fb_info_t *fb = brights_fb_get_info();
    if (!fb) return;
    
    int screen_width = fb->width;
    int screen_height = fb->height;
    
    brights_color_t bg_color = {208, 255, 255, 255};
    brights_color_t fg_color = {0, 0, 0, 255};
    brights_color_t sel_fg = {255, 255, 255, 255};
    brights_color_t sel_bg = {170, 0, 0, 255};
    brights_color_t gray = {136, 136, 136, 255};
    
    int y = screen_height - FONT_HEIGHT - 2;
    
    brights_fb_fill_rect(0, y, screen_width, FONT_HEIGHT + 4, bg_color);
    
    if (pinyin_len > 0) {
        brights_font_draw_string(4, y + 2, pinyin_buf, fg_color.r | (fg_color.g << 8) | (fg_color.b << 16), bg_color.r | (bg_color.g << 8) | (bg_color.b << 16));
        
        char pinyin_display[IM_MAX_PINYIN + 4];
        int pd = 0;
        while (pd < pinyin_len && pd < IM_MAX_PINYIN) {
            pinyin_display[pd] = pinyin_buf[pd];
            pd++;
        }
        pinyin_display[pd++] = ' ';
        pinyin_display[pd++] = ' ';
        pinyin_display[pd] = 0;
        
        int x_offset = brights_font_string_width(pinyin_display) + 8;
        
        for (int i = 0; i < candidate_count; i++) {
            int x = x_offset + i * IM_CANDIDATE_WIDTH;
            if (x + IM_CANDIDATE_WIDTH > screen_width) break;
            
            uint32_t fg = fg_color.r | (fg_color.g << 8) | (fg_color.b << 16);
            uint32_t bg = bg_color.r | (bg_color.g << 8) | (bg_color.b << 16);
            
            if (i == selected_candidate) {
                uint32_t sfg = sel_fg.r | (sel_fg.g << 8) | (sel_fg.b << 16);
                uint32_t sbg = sel_bg.r | (sel_bg.g << 8) | (sel_bg.b << 16);
                brights_fb_fill_rect(x, y + 2, IM_CANDIDATE_WIDTH - 4, FONT_HEIGHT, sel_bg);
                brights_font_draw_string(x + 4, y + 2, candidates[i], sfg, sbg);
            } else {
                char num[4];
                num[0] = '1' + i;
                num[1] = '.';
                num[2] = 0;
                uint32_t gr = gray.r | (gray.g << 8) | (gray.b << 16);
                brights_font_draw_string(x + 2, y + 2, num, gr, bg);
                brights_font_draw_string(x + 20, y + 2, candidates[i], fg, bg);
            }
        }
    }
}
