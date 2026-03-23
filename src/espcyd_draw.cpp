#include "espcyd.h"


/* externs from main.cpp */
extern volatile uint8_t myNodeID[4];
extern bool wifi_connected;

keyPadButtons_t buttons[4] = {
    {10,  50,  145, 70, "LIGHTS", 0, TFT_BLUE},
    {165, 50,  145, 70, "WIPERS", 1, TFT_DARKGREEN},
    {10,  130, 145, 70, "HORN",   2, TFT_RED},
    {165, 130, 145, 70, "AUX",    3, TFT_ORANGE}
};

/**
 * @brief Draws the hamburger icon in the top-right
 */
void drawHamburgerIcon() 
{
    int x = 280;
    int y = 12;
    tft.fillRect(x, y, 25, 4, TFT_WHITE);
    tft.fillRect(x, y + 8, 25, 4, TFT_WHITE);
    tft.fillRect(x, y + 16, 25, 4, TFT_WHITE);
}

/**
 * @brief Draws a stylized LED lightbar
 */
void drawLightbarIcon(int x, int y) 
{
    /* Main housing */
    tft.drawRoundRect(x - 12, y - 6, 24, 12, 2, TFT_WHITE);
    /* Individual LED "cells" */
    for (int i = 0; i < 3; i++) {
        tft.fillRect(x - 9 + (i * 7), y - 3, 5, 6, TFT_YELLOW);
    }
}

/**
 * @brief Draws a seat warmer (Seat profile with heat waves)
 */
void drawSeatWarmerIcon(int x, int y) 
{
    /* Seat Profile */
    tft.drawLine(x - 8, y - 5, x - 8, y + 8, TFT_WHITE); // Backrest
    tft.drawLine(x - 8, y + 8, x + 8, y + 8, TFT_WHITE); // Bottom
    
    /* Heat waves (squiggles) */
    tft.drawFastVLine(x - 2, y - 8, 4, TFT_RED);
    tft.drawFastVLine(x + 3, y - 8, 4, TFT_RED);
    tft.drawFastVLine(x + 8, y - 8, 4, TFT_RED);
}

/**
 * @brief Draws a water pump (Centrifugal housing style)
 */
void drawWaterPumpIcon(int x, int y) 
{
    /* Main circular body */
    tft.drawCircle(x, y, 8, TFT_WHITE);
    /* Outlet pipe */
    tft.fillRect(x + 4, y - 10, 6, 4, TFT_WHITE);
    /* Internal impeller cross */
    tft.drawFastHLine(x - 4, y, 8, TFT_CYAN);
    tft.drawFastVLine(x, y - 4, 8, TFT_CYAN);
}

/**
 * @brief Draws a windshield defroster (Curved pane with rising heat)
 */
void drawDefrosterIcon(int x, int y) 
{
    /* Curved windshield base */
    tft.drawEllipse(x, y + 8, 14, 4, TFT_WHITE);
    tft.fillRect(x - 14, y + 8, 28, 5, TFT_BLACK); // Mask bottom half of ellipse
    
    /* Rising heat lines */
    for (int i = 0; i < 3; i++) {
        int xOff = -8 + (i * 8);
        tft.drawLine(x + xOff, y + 4, x + xOff + 2, y - 4, TFT_ORANGE);
    }
}



/**
 * @brief Draws a color palette icon in the header (x=27)
 */
void drawPickerIcon() 
{
    /* Rainbow-ish 16x16 icon */
    tft.fillRect(27, 12, 8, 8, TFT_RED);
    tft.fillRect(35, 12, 8, 8, TFT_YELLOW);
    tft.fillRect(27, 20, 8, 8, TFT_BLUE);
    tft.fillRect(35, 20, 8, 8, TFT_GREEN);
    tft.drawRect(26, 11, 18, 18, TFT_WHITE);
}

/**
 * @brief Draws the footer with IP address and NodeID
 */
void drawFooter() 
{
    /* Erase footer area */
    tft.fillRect(0, 210, 320, 30, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);

    /* Format NodeID as Hex string (e.g., DEADBEEF) */
    char nodeStr[20];
    sprintf(nodeStr, "ID: %02X%02X%02X%02X", myNodeID[0], myNodeID[1], myNodeID[2], myNodeID[3]);

    /* Draw IP on left, NodeID on right */
    tft.drawString("IP: " + wifiIP, 10, 215, 2);
    tft.drawRightString(nodeStr, 310, 215, 2);
}

/**
 * @brief Draws the simplified header with mode toggle and hamburger menu.
 * @details This replaces the logic previously inside the 1000ms loop.
 */
void drawHeader(const char* title) 
{
    /* Clear header area with blue background */
    tft.fillRect(0, 0, 320, 43, TFT_BLUE);
    
    /* Left: Mode Toggle Icon (Home/Color) */
    drawPickerIcon(); /**< Color Picker Icon at x=27 */

    /* Center: Title and Selected Node context */
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.drawCentreString(title, 160, 10, 2);
    
    if (discoveredNodes[selectedNodeIdx].id != 0) {
        char nodeLbl[20];
        uint16_t txtCol = discoveredNodes[selectedNodeIdx].active ? TFT_WHITE : TFT_LIGHTGREY;
        tft.setTextColor(txtCol, TFT_BLUE);
        sprintf(nodeLbl, "Node: 0x%08X", discoveredNodes[selectedNodeIdx].id);
        tft.drawString(nodeLbl, 80, 28, 1);
    }

    /* Right: Hamburger Menu Icon */
    drawHamburgerIcon(); /**< Hamburger icon at x=280 */
}

/**
 * @brief Draws a 2x2 grid of buttons based on the provided items
 * @param title The header title for the screen
 * @param items Array of 4 GridItem structs
 */
void drawUnifiedGrid(const char* title, gridItem_t* items) 
{
    tft.fillScreen(TFT_BLACK);
    drawHeader(title);
    drawFooter();

    for (int i = 0; i < 4; i++) {
        int bx = buttons[i].x;
        int by = buttons[i].y;
        int bw = buttons[i].w;
        int bh = buttons[i].h;

        /* Draw Button Body */
        tft.fillRoundRect(bx, by, bw, bh, 8, items[i].color);
        tft.drawRoundRect(bx, by, bw, bh, 8, TFT_WHITE);
        
        /* Draw Icon (Upper half) */
        if (items[i].drawIcon != NULL) {
            items[i].drawIcon(bx + (bw / 2), by + (bh / 2) - 10);
        }

        /* Draw Label (Lower half) */
        tft.setTextColor(TFT_WHITE);
        tft.drawCentreString(items[i].label, bx + (bw / 2), by + bh - 22, 2);
    }
}

/**
 * @brief Draws a simple splash screen while waiting for CAN sync
 */
void drawSplashScreen(const char* message) 
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("INITIALIZING", 160, 100, 4);
    tft.drawCentreString(message, 160, 140, 2);
}

/**
 * @brief Draws a WiFi signal strength indicator in the top left
 * @param rssi The RSSI value from WiFi.RSSI()
 */
void drawWiFiStatus(int32_t rssi) 
{
    int x = 10;
    int y = 30;
    uint16_t color;

    /* Erase old indicator area in the header */
    tft.fillRect(x, 10, 30, 25, TFT_BLUE);

    /* Determine color based on strength */
    if (rssi > -67) color = TFT_GREEN;       /* Good */
    else if (rssi > -80) color = TFT_YELLOW; /* OK */
    else color = TFT_RED;                    /* Poor */

    /* Draw 4 bars of increasing height */
    for (int i = 0; i < 4; i++) {
        int barHeight = (i + 1) * 4;
        if (rssi > -90 + (i * 10)) {
            tft.fillRect(x + (i * 6), y - barHeight, 4, barHeight, color);
        } else {
            tft.drawRect(x + (i * 6), y - barHeight, 4, barHeight, TFT_WHITE);
        }
    }
}


void drawHomeIcon(int x, int y) 
{
    tft.fillTriangle(x, y-15, x-12, y, x+12, y, TFT_WHITE); // Roof
    tft.fillRect(x-8, y, 16, 12, TFT_WHITE);               // Body
    tft.fillRect(x-2, y+4, 4, 8, TFT_BLUE);                // Door
}

void drawPaletteIcon(int x, int y) 
{
    tft.fillCircle(x, y, 12, TFT_WHITE);
    tft.fillCircle(x-4, y-4, 3, TFT_RED);
    tft.fillCircle(x+4, y-4, 3, TFT_GREEN);
    tft.fillCircle(x, y+5, 3, TFT_BLUE);
}

void drawNetworkIcon(int x, int y) 
{
    tft.fillCircle(x, y-8, 4, TFT_WHITE);    // Top Node
    tft.fillCircle(x-8, y+8, 4, TFT_WHITE);  // Left Node
    tft.fillCircle(x+8, y+8, 4, TFT_WHITE);  // Right Node
    tft.drawLine(x, y-4, x-6, y+6, TFT_WHITE);
    tft.drawLine(x, y-4, x+6, y+6, TFT_WHITE);
}

void drawInfoIcon(int x, int y) 
{
    tft.fillCircle(x, y, 12, TFT_WHITE);
    tft.setTextColor(TFT_NAVY);
    tft.drawCentreString("i", x, y - 6, 2); // Simple 'i' for info
}

void drawColorPicker() 
{
    int swatchW = 40;
    int swatchH = 45;
    int startY = 45;
    drawHeader("COLOR PICKER");

    /* Get the currently active color for the selected node */
    int activeIdx = discoveredNodes[selectedNodeIdx].lastColorIdx;

    for (int i = 0; i < 32; i++) {
        int col = i % 8;
        int row = i / 8;
        int x = col * swatchW;
        int y = startY + (row * swatchH);
        PaletteColor pColor = SystemPalette[i];

        uint16_t color565 = colorTo565(pColor);
        tft.fillRect(x, y, swatchW, swatchH, color565);
        
        /* Draw selection highlight if this is the active color */
        if (i == activeIdx) {
            tft.drawRect(x, y, swatchW, swatchH, TFT_RED);
            tft.drawRect(x + 1, y + 1, swatchW - 2, swatchH - 2, TFT_RED); // Thick 2px border
        } else {
            tft.drawRect(x, y, swatchW, swatchH, TFT_WHITE);
        }
    }
}

void drawHamburgerMenu() 
{
    gridItem_t menuItems[4] = {
        {"HOME",    TFT_BLUE,       drawHomeIcon},
        {"COLORS",  TFT_DARKGREEN,  drawPaletteIcon},
        {"NODES",   TFT_MAROON,     drawNetworkIcon},
        {"SYSTEM",  TFT_NAVY,       drawInfoIcon}
    };
    drawUnifiedGrid("MAIN MENU", menuItems);
}

void drawKeypad() 
{
    gridItem_t keypadItems[4] = {
        {"LT BAR", TFT_BLUE,       drawLightbarIcon},
        {"SEAT WARMER",   TFT_MAROON,     drawSeatWarmerIcon},
        {"WTR PUMP",     TFT_DARKGREEN,  drawWaterPumpIcon},
        {"DEFROST",  TFT_ORANGE,     drawDefrosterIcon}
    };
    
    drawUnifiedGrid("VEHICLE CONTROL", keypadItems);
}

/**
 * @brief Draws the node selection screen with color-coded node states.
 * - Yellow: Currently selected node.
 * - White: Active (but not selected) node.
 * - Light Grey: Inactive node.
 */
void drawNodeSelector() 
{
    /* 1. Draw the standard blue header */
    // tft.fillRect(0, 0, 320, 43, TFT_BLUE);
    // tft.setTextColor(TFT_WHITE, TFT_BLUE);
    // tft.drawCentreString("SELECT TARGET NODE", 160, 10, 2);
    drawHeader("SELECT TARGET NODE");
    // drawPickerIcon();
    // drawHamburgerIcon();

    /* 2. Draw the Content Area */
    tft.fillRect(0, 44, 320, 196, TFT_BLACK);

 /* 2. Content Area Setup */
    const uint16_t headerHeight = 44;               /**< Space reserved for header */
    const uint16_t contentHeight = 240 - headerHeight; /**< 196 pixels remaining */
    
    const uint8_t  cols = 4;
    const uint8_t  rows = 2;
    const uint16_t btnW = 320 / cols;               /**< 80 pixels wide */
    const uint16_t btnH = contentHeight / rows;     /**< 98 pixels high */

    tft.fillRect(0, headerHeight, 320, contentHeight, TFT_BLACK);

    for (int i = 0; i < discoveredNodeCount; i++) {
        /* Calculate grid coordinates relative to content area */
        int col = i % cols;
        int row = i / cols;
        int x = col * btnW;
        int y = headerHeight + (row * btnH);

        /* Truncate Node ID to last 2 bytes */
        char label[8];
        sprintf(label, "0x%04X", (uint16_t)(discoveredNodes[i].id & 0xFFFF));

        /* Resolve background color from the saved index */
        uint16_t bgColor = TFT_BLACK;
        int idx = discoveredNodes[i].lastColorIdx;
        if (idx >= 0 && idx < COLOR_PALETTE_SIZE) {
            bgColor = colorTo565(SystemPalette[idx]);
        }

        /* Draw Button Body */
        tft.fillRect(x + 2, y + 2, btnW - 4, btnH - 4, bgColor);
        
        /* Contrast border and selection highlight */
        uint16_t borderColor = (i == selectedNodeIdx) ? TFT_YELLOW : 
                               (bgColor < 0x2104) ? TFT_DARKGREY : TFT_WHITE;
        
        tft.drawRect(x + 2, y + 2, btnW - 4, btnH - 4, borderColor);
        if (i == selectedNodeIdx) {
            tft.drawRect(x + 3, y + 3, btnW - 6, btnH - 6, TFT_YELLOW); /**< Thicker highlight */
        }

        /* Text Contrast Logic */
        uint16_t textColor = (bgColor > 0x7BEF) ? TFT_BLACK : TFT_WHITE;
        tft.setTextColor(textColor);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(label, x + (btnW / 2), y + (btnH / 2), 2);
    }
    
    /* 3. Footer hint */
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("Tap ID to select target", 160, 225, 1);
}


/**
 * @brief Draws diagnostic info including the relocated clock and CAN metrics.
 */
void drawSystemInfo() 
{
    tft.fillScreen(TFT_BLACK);
    drawHeader("SYSTEM INFO");

    /* --- Relocated Clock --- */
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString(timeStr, 160, 60, 4); 
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString("System Time (UTC/Local)", 160, 95, 1);
    }

    /* --- Detailed CAN & Network Metrics --- */
    twai_status_info_t status;
    int yPos = 120;
    
    if (twai_get_status_info(&status) == ESP_OK) {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("CAN BUS STATUS:", 20, yPos, 2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        
        tft.setCursor(30, yPos + 20);
        tft.printf("State: %s", (status.state == TWAI_STATE_RUNNING) ? "RUNNING" : "ERROR");
        tft.setCursor(30, yPos + 40);
        tft.printf("TX Errs: %d | RX Errs: %d", status.tx_error_counter, status.rx_error_counter);
    }

    /* Network Info */
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("NETWORK:", 20, yPos + 70, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(30, yPos + 90);
    tft.printf("IP: %s", wifiIP.c_str());
    tft.setCursor(30, yPos + 110);
    tft.printf("RSSI: %d dBm", WiFi.RSSI());
}

/**
 * @brief Redraws the current screen based on the active mode. Keep this function below other draw functions
 */
void refreshCurrentScreen() 
{
    switch(currentMode) {
        case MODE_HOME:           drawKeypad();        break;
        case MODE_COLOR_PICKER:   drawColorPicker();   break;
        case MODE_NODE_SEL:       drawNodeSelector();  break;
        case MODE_SYSTEM_INFO:    drawSystemInfo();    break;
        case MODE_HAMBURGER_MENU: drawHamburgerMenu(); break;
    }
}