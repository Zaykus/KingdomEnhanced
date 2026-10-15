#include "i18n.h"
#include <unordered_map>
#include <string>

static Lang g_currentLang = Lang::English;

struct Entry {
    const char* en;
    const char* zh;
    const char* ko;
    const char* es;
    const char* de;
    const char* fr;
};

static const std::unordered_map<std::string, Entry> kDict = {
    // Tabs
    { "TAB_MAIN",       { "[ MAIN ]",       "[ 首页 ]",     "[ 메인 ]",      "[ PRINCIPAL ]",   "[ HAUPT ]",       "[ ACCUEIL ]" } },
    { "TAB_CHEATS",     { "[ CHEATS ]",     "[ 作弊 ]",     "[ 치트 ]",      "[ TRUCOS ]",      "[ CHEATS ]",      "[ TRICHE ]" } },
    { "TAB_SPAWNER",    { "[ SPAWNER ]",    "[ 生成器 ]",   "[ 소환기 ]",    "[ GENERADOR ]",   "[ SPAWNER ]",     "[ SPAWNER ]" } },
    { "TAB_LAB",        { "[ LAB ]",        "[ 实验 ]",     "[ 연구소 ]",    "[ LAB ]",         "[ LABOR ]",       "[ LABO ]" } },
    { "TAB_HARD",       { "[ HARD ]",       "[ 困难 ]",     "[ 하드 ]",      "[ DIFÍCIL ]",     "[ SCHWER ]",      "[ DIFFICILE ]" } },
    { "TAB_MONITOR",    { "[ MONITOR ]",    "[ 监控 ]",     "[ 모니터 ]",    "[ MONITOR ]",     "[ MONITOR ]",     "[ STATS ]" } },
    { "TAB_SETTINGS",   { "[ SETTINGS ]",   "[ 设置 ]",     "[ 설정 ]",      "[ AJUSTES ]",     "[ EINSTELLUNGEN ]","[ PARAMÈTRES ]" } },
    { "TAB_LOGS",       { "[ LOGS ]",       "[ 日志 ]",     "[ 로그 ]",      "[ REGISTROS ]",   "[ PROTOKOLL ]",   "[ JOURNAUX ]" } },

    // Sections & Titles
    { "MENU_TITLE",     { "KINGDOM ENHANCED", "王国增强",   "킹덤 인핸스드", "REINO MEJORADO", "KINGDOM ENHANCED", "ROYAUME AMÉLIORÉ" } },
    { "SEC_ESSENTIALS", { "QUICK ESSENTIALS", "快捷核心",   "빠른 기본 기능", "ESENCIALES RÁPIDOS", "SCHNELLE FUNKTIONEN", "ESSENTIELS RAPIDES" } },
    { "SEC_HOTKEYS",    { "ACTIVE HOTKEYS",   "生效快捷键", "활성 단축키",   "ATAJOS ACTIVOS",  "TASTATURKÜRZEL",   "RACCOURCIS ACTIFS" } },
    { "SEC_INVINCIBLE", { "INVINCIBILITY & STATUS", "无敌与状态", "무적 및 상태", "INVENCIBILIDAD Y ESTADO", "UNVERWUNDBARKEIT & STATUS", "INVINCIBILITÉ ET STATUT" } },
    { "SEC_MOVEMENT",   { "MOVEMENT & SCALING", "移动与缩放", "이동 및 크기", "MOVIMIENTO Y ESCALA", "BEWEGUNG & SKALIERUNG", "DÉPLACEMENT & ÉCHELLE" } },
    { "SEC_ARTEMIS",    { "ARTEMIS BOW",     "阿尔忒弥斯之弓", "아르테미스의 활", "ARCO DE ARTEMISA", "ARTEMIS-BOGEN", "ARC D'ARTÉMIS" } },
    { "SEC_ECONOMY",    { "ECONOMY",         "经济与货币",   "경제",         "ECONOMÍA",        "WIRTSCHAFT",      "ÉCONOMIE" } },
    { "SEC_MILITARY",   { "MILITARY",        "军事与战斗",   "군사",         "MILITAR",         "MILITÄR",         "MILITAIRE" } },
    { "SEC_BUILDERS",   { "CONSTRUCTION & BUILDERS", "建筑与工匠", "건설 및 일꾼", "CONSTRUCCIÓN Y CONSTRUCTORES", "BAU & ARBEITER", "CONSTRUCTION & BÂTISSEURS" } },
    { "SEC_SPAWN_CIT",  { "SPAWN CITIZENS & ROLES", "生成平民与职业", "시민 및 직업 소환", "GENERAR CIUDADANOS", "BÜRGER SPAWNEN", "GÉNÉRER DES CITOYENS" } },
    { "SEC_SPAWN_HERM", { "SPAWN HERMITS",   "生成隐士",     "은둔자 소환",  "GENERAR HERMITAÑOS", "EINSIEDLER SPAWNEN", "GÉNÉRER DES ERMITES" } },
    { "SEC_SPAWN_GREED",{ "SPAWN GREED ENEMIES", "生成贪婪敌人", "그리드 적 소환", "GENERAR ENEMIGOS GREED", "GREED-FEINDE SPAWNEN", "GÉNÉRER DES ENNEMIS GREED" } },
    { "SEC_TIME_SEASON",{ "TIME & SEASON CONTROL", "时间与季节控制", "시간 및 계절 제어", "CONTROL DE TIEMPO Y ESTACIÓN", "ZEIT & JAHRESZEIT", "CONTRÔLE DU TEMPS & SAISON" } },
    { "SEC_WORLD_BUFF", { "WORLD BUFFS & DEFENSES", "世界增益与防御", "월드 버프 및 방어", "MEJORAS DE MUNDO Y DEFENSA", "WELT-BUFFS & VERTEIDIGUNG", "BUFFS DU MONDE & DÉFENSE" } },
    { "SEC_SIEGE",      { "SIEGE & RULER BUFFS", "攻城与君主增益", "공성 및 군주 버프", "ASEDIO Y MEJORAS DE GOBERNANTE", "BELAGERUNG & HERRSCHER", "SIÈGE & BUFFS DU SOUVERAIN" } },
    { "SEC_WAVE_SCALING",{ "WAVE SCALING & PORTALS", "波次倍率与传送门", "웨이브 배율 및 포털", "ESCALADO DE OLEADAS Y PORTALES", "WELLEN-SKALIERUNG & PORTALE", "ÉCHELLE DES VAGUES & PORTAILS" } },
    { "SEC_DIFFICULTY", { "DIFFICULTY MULTIPLIERS", "难度与系数调整", "난이도 배율", "MULTIPLICADORES DE DIFICULTAD", "SCHWIERIGKEITS-MULTIPLIKATOREN", "MULTIPLICATEURS DE DIFFICULTÉ" } },
    { "SEC_RULES",      { "RULES & RESTRICTIONS", "规则与限制", "규칙 및 제한", "REGLAS Y RESTRICCIONES", "REGELN & BESCHRÄNKUNGEN", "RÈGLES & RESTRICTIONS" } },
    { "SEC_LANGUAGE",   { "LANGUAGE",        "语言选择",     "언어 선택",    "IDIOMA",          "SPRACHE",         "LANGUE" } },
    { "SEC_MODULES",    { "MODULE TOGGLES",  "模块开关",     "모듈 토글",    "MÓDULOS",         "MODULE",          "MODULES" } },
    { "SEC_DISPLAY",    { "DISPLAY / 4K SCALING", "显示与4K缩放", "화면 및 4K 배율", "PANTALLA / ESCALADO 4K", "ANZEIGE / 4K-SKALIERUNG", "AFFICHAGE / ÉCHELLE 4K" } },
    { "SEC_SAVE_LOAD",  { "SAVE / LOAD",     "保存与载入",   "저장 / 불러오기", "GUARDAR / CARGAR", "SPEICHERN / LADEN", "SAUVEGARDER / CHARGER" } },

    // Actions & Cheats
    { "BTN_DROP_10_C",  { "+10 Coins",       "+10 金币",     "+10 코인",     "+10 Monedas",     "+10 Münzen",      "+10 Pièces" } },
    { "BTN_DROP_50_C",  { "+50 Coins",       "+50 金币",     "+50 코인",     "+50 Monedas",     "+50 Münzen",      "+50 Pièces" } },
    { "BTN_ADD_5_GEMS", { "+5 Gems",         "+5 宝石",      "+5 보석",      "+5 Gemas",        "+5 Edelsteine",   "+5 Gemmes" } },
    { "BTN_FILL_WALLET",{ "Fill Wallet",     "装满钱包",     "지갑 채우기",  "Llenar Monedero", "Geldbörse Füllen", "Remplir Bourse" } },
    { "BTN_CLEAR_COINS",{ "Clear Coins",     "清理散落金币", "바닥 코인 청소", "Limpiar Monedas", "Münzen Säubern",  "Vider Pièces" } },
    { "BTN_LAUNCH_BOULDER",{ "Launch Catapult Boulder (Space)", "发射投石车巨石 (空格键)", "투석기 바위 발사 (스페이스바)", "Lanzar Roca Catapulta (Espacio)", "Katapult-Stein Werfen (Leertaste)", "Lancer Rocher Catapulte (Espace)" } },
    { "BTN_KILL_ENEMIES",{ "Kill All Enemies", "消灭所有敌人", "모든 적 처치", "Matar Enemigos",  "Alle Feinde Töten", "Tuer Tous Ennemis" } },
    { "BTN_DESTROY_PORTALS",{ "Destroy Portals", "摧毁传送门", "포털 파괴",  "Destruir Portales", "Portale Zerstören", "Détruire Portails" } },
    { "BTN_RECRUIT_VAGRANTS",{ "Recruit Vagrants", "招募所有流民", "방랑자 고용", "Reclutar Vagabundos", "Vagabunden Anwerben", "Recruter Vagabonds" } },
    { "BTN_MAX_ARMY",   { "Max Army",        "组建精锐军队", "군대 최대치",  "Ejército Máximo", "Maximale Armee",  "Armée Maximale" } },
    { "BTN_DROP_BOW",   { "Drop Archer Bow", "掉落弓箭",     "활 드롭",      "Soltar Arco",     "Bogen Fallenlassen", "Déposer Arc" } },
    { "BTN_DROP_HAMMER",{ "Drop Builder Hammer", "掉落工匠铁锤", "망치 드롭", "Soltar Martillo", "Hammer Fallenlassen", "Déposer Marteau" } },
    { "BTN_SKIP_PHASE", { "Skip Day/Night Phase", "跳过昼夜阶段", "주야간 넘기기", "Saltar Fase Día/Noche", "Tag/Nacht-Phase Überspringen", "Passer Phase Jour/Nuit" } },
    { "BTN_IND_WALLS",  { "Make Walls Indestructible", "城墙全满耐久", "성벽 완전 수리", "Muros Indestructibles", "Unzerstörbare Mauern", "Murs Indestructibles" } },

    // Checkboxes
    { "CHK_GOD_MODE",   { "God Mode",        "无敌模式",     "무적 모드",    "Modo Dios",       "Gott-Modus",      "Mode Dieu" } },
    { "CHK_INF_STAMINA",{ "Infinite Stamina", "无限耐力",     "무한 스태미나", "Estamina Infinita", "Unendliche Ausdauer", "Endurance Infinie" } },
    { "CHK_NO_TOOL_CD", { "No Tool Cooldowns", "无工具冷却", "도구 쿨다운 제거", "Sin Recarga de Herramienta", "Keine Abklingzeit", "Pas de Temps de Recharge" } },
    { "CHK_INSTANT_BUILD",{ "Instant Construction", "瞬间建造", "즉시 건설", "Construcción Instantánea", "Sofortiges Bauen", "Construction Instantanée" } },
    { "CHK_LARGE_CAMPS",{ "Expand Vagrant Camps", "扩大流民营地", "난민 캠프 확장", "Ampliar Campamentos", "Lager Erweitern", "Agrandir Camps" } },
    { "CHK_DISABLE_BLOOD",{ "Disable Blood Moon", "禁用血月", "블러드문 비활성화", "Desactivar Luna de Sangre", "Blutmond Deaktivieren", "Désactiver Lune de Sang" } },
    { "CHK_RAPID_ARCHERS",{ "Rapid-Fire Archers", "弓箭手高速射击", "궁수 연사 속도 강화", "Arqueros Rápidos", "Schnellfeuer-Bogenschützen", "Archers Tir Rapide" } },
    { "CHK_NO_CROWN_STEAL",{ "Prevent Greed Crown Stealing", "防止贪婪偷夺王冠", "왕관 탈취 방지", "Evitar Robo de Corona", "Kronenraub Verhindern", "Empêcher Vol de Couronne" } },
    { "CHK_MINIMAP",    { "Minimap",         "顶部小地图",   "미니맵",       "Minimapa",        "Minikarte",       "Mini-carte" } },
    { "CHK_STAMINA_BAR",{ "Stamina Bar",     "坐骑耐力条",   "스태미나 바",  "Barra de Estamina", "Ausdauerleiste",  "Barre d'Endurance" } },
    { "CHK_HOTKEYS",    { "Hotkeys",         "快捷键功能",   "단축키",       "Atajos",          "Tastenkürzel",    "Raccourcis" } },
    { "CHK_BOLD_FONT",  { "Bolder Text Font", "加粗字体显示", "굵은 글꼴",   "Fuente en Negrita", "Fette Schriftart", "Police Plus Épaisse" } },

    // Sliders
    { "SLD_TRAVEL_SPEED",{ "Travel Speed",    "移动速度",     "이동 속도",    "Velocidad de Viaje", "Reisegeschwindigkeit", "Vitesse de Déplacement" } },
    { "SLD_PLAYER_SCALE",{ "Player Scale",    "角色体型",     "캐릭터 크기",  "Escala del Jugador", "Spielergröße",    "Échelle du Joueur" } },
    { "SLD_COIN_INCOME", { "Coin Income Multiplier", "金币收入倍率", "수입 배율", "Multiplicador de Monedas", "Münzeinkommen-Multiplikator", "Multiplicateur de Revenu" } },
    { "SLD_BUILDER_SPEED",{ "Builder Movement Speed", "工匠移动速度", "일꾼 이동 속도", "Velocidad de Constructores", "Arbeiter-Tempo", "Vitesse des Bâtisseurs" } },
    { "SLD_BUILDER_EFF", { "Builder Work Efficiency", "工匠工作效率", "일꾼 작업 효율", "Eficiencia de Constructores", "Arbeitseffizienz", "Efficacité des Bâtisseurs" } },
    { "SLD_WAVE_SIZE",   { "Wave Size Multiplier", "敌袭波次数量倍率", "웨이브 크기 배율", "Multiplicador de Oleada", "Wellen-Größe", "Multiplicateur de Vague" } },
    { "SLD_ENEMY_SPEED", { "Enemy Speed Multiplier", "敌人移动速度倍率", "적 이동 속도 배율", "Velocidad de Enemigos", "Feind-Geschwindigkeit", "Vitesse des Ennemis" } },
    { "SLD_PORTAL_SPAWN",{ "Portal Spawn Rate", "传送门生成速率", "포털 소환 주기", "Tasa de Aparición Portal", "Portal-Spawn-Rate", "Taux d'Apparition Portails" } },
    { "SLD_DIRECTOR",    { "AI Director Threat", "AI 威胁指数", "AI 위협 지수", "Amenaza Director IA", "KI-Bedrohung", "Menace Directeur IA" } },
    { "SLD_UI_SCALE",    { "Interface Scaling", "界面缩放倍率", "UI 배율", "Escala de Interfaz", "UI-Skalierung", "Échelle d'Interface" } },
    { "SLD_HUD_ALPHA",   { "HUD Translucency (Alpha)", "监控悬浮窗透明度", "HUD 투명도", "Transparencia del HUD", "HUD-Transparenz", "Translucidité du HUD" } },

    // Buttons
    { "BTN_SAVE_CONFIG", { "Save Config",    "保存配置",     "설정 저장",    "Guardar Ajustes", "Einstellungen Speichern", "Enregistrer Config" } },
    { "BTN_LOAD_CONFIG", { "Load Config",    "载入配置",     "설정 불러오기", "Cargar Ajustes",  "Einstellungen Laden", "Charger Config" } },
    { "BTN_CLEAR_LOGS",  { "Clear Logs",     "清空日志",     "로그 지우기",  "Limpiar Registros", "Protokoll Leeren", "Vider Journaux" } },
    { "BTN_COPY_LOGS",   { "Copy All to Clipboard", "复制全部日志到剪贴板", "클립보드에 복사", "Copiar al Portapapeles", "In Zwischenablage Kopieren", "Copier dans Presse-papier" } },
    { "BTN_OPEN_LOG_FILE",{ "Open Log File", "打开日志文件", "로그 파일 열기", "Abrir Archivo Log", "Protokolldatei Öffnen", "Ouvrir Fichier Log" } }
};

void SetLanguage(Lang lang) {
    g_currentLang = lang;
}

Lang GetLanguage() {
    return g_currentLang;
}

const char* TR(const char* key) {
    if (!key) return "";
    auto it = kDict.find(key);
    if (it == kDict.end()) return key;

    const char* str = nullptr;
    switch (g_currentLang) {
        case Lang::Chinese: str = it->second.zh; break;
        case Lang::Korean:  str = it->second.ko; break;
        case Lang::Spanish: str = it->second.es; break;
        case Lang::German:  str = it->second.de; break;
        case Lang::French:  str = it->second.fr; break;
        case Lang::English:
        default:            str = it->second.en; break;
    }
    return str ? str : it->second.en;
}

const char* GetLanguageName(Lang lang) {
    switch (lang) {
        case Lang::Chinese: return "Simplified Chinese";
        case Lang::Korean:  return "Korean";
        case Lang::Spanish: return "Español";
        case Lang::German:  return "Deutsch";
        case Lang::French:  return "Français";
        case Lang::English:
        default:            return "English";
    }
}
