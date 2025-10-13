<!-- 019ac123-a5b4-459f-90d0-1888dea53b60 8e9c3cbf-61f2-43d9-ab10-efc7e8d20cb8 -->
# Phase 4: Matter → UART Integration

## Overview

Connect the C3's Matter device callbacks to UART commands, enabling control from HomeKit. When someone triggers the device in Apple Home, it sends UART commands to the S3. Mode changes via brightness slider also propagate via UART.

## Design Decisions (Confirmed)

✅ **Mode Selector:** Option A - Dimmable Light

- Single dimmable light endpoint (already exists!)
- Brightness ranges map to modes:
  - 1-24% → Mode 0 (Little Kid) - DEFAULT
  - 25-49% → Mode 1 (Big Kid)  
  - 50-74% → Mode 2 (Take One)
  - 75-100% → Mode 3 (Closed)
- Results in 2 tiles total in Home app (trigger + mode)

✅ **S3 Skit Logic:** Placeholder only

- Just logs commands
- Returns ACK immediately
- Optional: Add LED blinks for visual feedback

✅ **Mode Persistence:** None

- Resets to Mode 0 (Little Kid) on every boot
- No NVS storage needed

## Implementation Plan

### Part 1: C3 Matter → UART Integration

**File:** `esp32-matter-node/firmware/main/app_main.cpp`

**Changes Required:**

**1. Add LevelControl Cluster to Light Endpoint**

In `app_main()` where we create the light endpoint (~line 656):

- The light endpoint already exists with OnOff
- Add LevelControl cluster to it
- Initialize to level 12 (middle of mode 0 range: 1-24%)

**2. Modify `app_attribute_update_cb()` function (~line 444)**

Add two new detection blocks:

**Block A - Trigger Detection:**

```cpp
// Detect trigger switch (endpoint 1) turning ON
if (endpoint_id == g_switch_endpoint_id && 
    cluster_id == OnOff::Id && 
    attribute_id == OnOff::Attributes::OnOff::Id) {
    
    if (val->val.b == true) {  // Switch turned ON
        ESP_LOGI(TAG, "HomeKit TRIGGER detected - sending UART command");
        uart_send_response(CMD_TRIGGER);  // Reuse existing function
        // Note: GPIO pulse and auto-reset already handled by existing code
    }
}
```

**Block B - Mode Detection:**

```cpp
// Detect brightness/level change on light endpoint
if (endpoint_id == g_ui_endpoint_id && 
    cluster_id == LevelControl::Id && 
    attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
    
    uint8_t level = val->val.u8;  // 0-254 range
    
    // Map to mode (0-3)
    uint8_t mode;
    if (level < 61) mode = 0;        // 0-24% → mode 0
    else if (level < 127) mode = 1;  // 25-49% → mode 1
    else if (level < 190) mode = 2;  // 50-74% → mode 2
    else mode = 3;                   // 75-100% → mode 3
    
    if (mode != g_current_mode) {
        g_current_mode = mode;
        ESP_LOGI(TAG, "Mode changed to %d via HomeKit - sending UART", mode);
        uint8_t payload[1] = { mode };
        uart_send_response(CMD_SET_MODE, payload, 1);  // Reuse existing function
        // LED feedback already handled by UART response handler
    }
}
```

**3. Initialize Light Endpoint with LevelControl**

In `app_main()` after creating the light endpoint:

- Add LevelControl cluster
- Set initial level to 12 (mode 0 default)
- Ensure OnOff is false initially

### Part 2: S3 Visual Feedback (Optional Enhancement)

**File:** `esp32-matter-master/esp32-matter-master.ino`

**Simple additions for better testing:**

**1. Add LED Pin Definition (~line 10)**

```cpp
#define LED_BUILTIN 2  // Built-in LED on most ESP32-S3 boards
```

**2. Add LED Helper Functions (~line 50)**

```cpp
void ledBlink(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(ms);
    digitalWrite(LED_BUILTIN, LOW);
    delay(ms);
  }
}
```

**3. Initialize LED in setup() (~line 275)**

```cpp
pinMode(LED_BUILTIN, OUTPUT);
digitalWrite(LED_BUILTIN, LOW);
```

**4. Add Visual Feedback in Command Handlers**

```cpp
// In cmdHello() - blink once
ledBlink(1, 200);

// In cmdPing() - blink twice
ledBlink(2, 100);

// In cmdTrigger() - blink long
digitalWrite(LED_BUILTIN, HIGH);
delay(500);
digitalWrite(LED_BUILTIN, LOW);

// In cmdSetMode() - blink (mode+1) times
ledBlink(mode + 1, 150);
```

## Testing Plan

### Test 1: Trigger via HomeKit

1. Open Apple Home app
2. Find Fortune Teller device
3. Turn trigger switch ON
4. **Expected:**

   - S3 Serial Monitor shows: `>>> Sending TRIGGER` + `✓ ACK received`
   - S3 LED blinks once long (500ms)
   - C3 logs: `HomeKit TRIGGER detected`
   - Switch auto-resets to OFF in Home app after 500ms

### Test 2: Mode Selection via Brightness

1. In Home app, tap the light tile
2. Slide brightness to different levels:

   - 10% → Should set mode 0 (Little Kid)
   - 40% → Should set mode 1 (Big Kid)
   - 60% → Should set mode 2 (Take One)
   - 90% → Should set mode 3 (Closed)

3. **Expected for each:**

   - C3 logs: `Mode changed to X via HomeKit`
   - S3 receives SET_MODE command
   - S3 LED blinks (X+1) times
   - S3 shows: `✓ ACK received`

### Test 3: Mode Reset on Reboot

1. Set mode to 3 via brightness slider
2. Power cycle the C3 (unplug/replug USB)
3. Check brightness in Home app after C3 reboots
4. **Expected:**

   - Brightness resets to ~5% (mode 0 default)
   - No mode stored - starts fresh

### Test 4: Combined Test

1. Set brightness to 60% (mode 2 - Take One)
2. Trigger the switch
3. **Expected:**

   - C3 sends SET_MODE 2 to S3
   - C3 sends TRIGGER to S3
   - S3 knows current mode is 2
   - Both commands acknowledged

## Code Changes Summary

**C3 Changes:**

```
app_main.cpp:
├── Line ~656: Add LevelControl cluster to light endpoint
├── Line ~444: Add trigger detection in app_attribute_update_cb()
├── Line ~460: Add mode detection in app_attribute_update_cb()
└── Line ~666: Initialize light level to 12 (mode 0)
```

**S3 Changes (Optional):**

```
esp32-matter-master.ino:
├── Line ~10: Add LED_BUILTIN definition
├── Line ~50: Add ledBlink() helper function
├── Line ~275: Initialize LED in setup()
├── Line ~200-240: Add LED feedback in command handlers
```

## Implementation Steps

1. **Add LevelControl cluster** to C3 light endpoint
2. **Add trigger detection** to attribute callback
3. **Add mode detection** to attribute callback (with level→mode mapping)
4. **Initialize defaults** (level=12, mode=0) in app_main()
5. **Build and flash C3** - test with existing UART CLI
6. **Add S3 LED feedback** (optional but helpful)
7. **Test via HomeKit** - verify all triggers and mode changes

## Success Criteria

- [x] UART communication already working (from Phase 1-3)
- [ ] Trigger switch in Home app sends UART TRIGGER to S3
- [ ] Brightness slider changes send SET_MODE to S3
- [ ] Correct mode mapping (brightness % → mode 0-3)
- [ ] S3 visual feedback shows commands received
- [ ] Mode resets to 0 on C3 reboot
- [ ] Switch auto-resets to OFF in Home app
- [ ] Both tiles work independently in Home app

## Notes

- Light endpoint already exists - just adding LevelControl cluster
- No new endpoints needed
- Reusing existing UART send/receive functions
- No NVS code needed (simplified!)
- S3 LED feedback optional but recommended for testing
- This proves the concept - final implementation will add real skit logic

### To-dos

- [ ] Decide: Option A (dimmable light) or Option B (four switches) for mode selector
- [ ] Decide: Keep S3 placeholder or add basic skit demo logic
- [ ] Add Matter OnOff callback → UART TRIGGER in app_attribute_update_cb()
- [ ] Add mode selector logic (dimmable or switches) → UART SET_MODE
- [ ] Implement NVS read/write for mode persistence
- [ ] Enhance UART response handling (BUSY/DONE states)
- [ ] Optional: Add basic skit playback logic to S3
- [ ] Test trigger switch in Home app → verify UART command sent
- [ ] Test mode changes in Home app → verify SET_MODE sent and persists
- [ ] Test BUSY response handling when triggering during active skit