## React Native DP-05 Project Tasks (Ongoing)

### 1. Environment Setup
- [x] Create React Native project `DP05App`.
- [x] Set up initial `App.tsx` layout with global styles.
- [x] Integrate `Geist` and `JetBrains Mono` fonts.
- [ ] Resolve Java/Android SDK environment issues (JAVA_HOME, Android SDK path, supported JDK version).
    - [x] Downgrade JDK to 20.0.2.
    - [x] Set `sdk.dir` in `DP05App/android/local.properties`.
    - [x] Set `org.gradle.java.home` in `DP05App/android/gradle.properties`.
    - [ ] **ACTION REQUIRED:** User to set `JAVA_HOME` system-wide to `C:\Program Files\Java\jdk-20` and restart PC.

### 2. UI Component Implementation (based on web app analysis)
- [ ] Recreate `NoiseOverlay` component.
- [ ] Recreate `DriftLayer` component with burn-in protection logic.
- [ ] Implement `ClockPanel` component (digital clock, colon blink).
- [ ] Implement `CalendarGrid` component.
- [ ] Implement `DayModule` component (day progress bar).
- [ ] Implement `RemainModule` component (countdown timer).
- [ ] Implement `NetModule` component (network stats, sparkline graph).
- [ ] Implement `DevicesModule` component (device list).
- [ ] Implement `SettingsPanel` component and long-press interaction.

### 3. Android-Specific Data Integration
- [ ] Integrate `react-native-device-info` for device model, OS, etc.
- [ ] Integrate `react-native-netinfo` for network status, IP addresses.
- [ ] Implement logic to get CPU/memory usage (if feasible and aligns with "interior app" concept).
- [ ] Adapt data fetching and real-time updates for Android APIs.

### 4. Styling Adaptation
- [ ] Translate neumorphic design (plate-concave, plate-convex) to React Native `StyleSheet` and potentially `react-native-svg` for complex shadows/gradients.
- [ ] Implement night mode and glow color themes.

### 5. Polish & Verification
- [ ] Cross-device testing.
- [ ] Performance optimization.
