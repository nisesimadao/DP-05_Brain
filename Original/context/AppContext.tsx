import React, { createContext, useContext } from 'react';

// Define the shape of your state
export interface TodoItem {
    id: string;
    text: string;
    completed: boolean;
    createdAt: number;
}

export interface AppState {
    customFontFamily: string | null;
    fontWeight: number;
    secondsScale: number;
    thinOverride: boolean;
    monoEnforce: boolean;
    nightMode: boolean;
    glowColor: 'green' | 'red' | 'amber';
    burninGuard: boolean;
    burninCrtMode: boolean;
    isFullscreen: boolean;
    settingsOpen: boolean;
    colonPhase: number;
    lastColonToggle: number;
    lastSecond: number;
    lastRemainSecond: number;
    monoWidthRatio: number;
    driftX: number;
    driftY: number;
    driftDirX: number;
    driftDirY: number; // Added this line
    driftTimer: number;
    driftCycle: number;
    lastDriftTime: number;
    unchangedTime: number;
    dlSpeed: number;
    ulSpeed: number;
    targetDl: number;
    targetUl: number;
    dlHistory: number[];
    ulHistory: number[];
    measuredDl: number;
    measuredUl: number;
    measureAvailable: boolean;
    longPressTimer: NodeJS.Timeout | null;
    isPressing: boolean;
    publicIp: string;
    localIp: string;
    todos: TodoItem[];
    // prevDevices: any; // This will be handled in DevicesModule if needed
}

// Define the shape of the context, including state and a function to update it
interface AppContextType {
    state: AppState;
    updateGlobalState: (newValues: Partial<AppState>) => void;
}

// Initial state (same as in Layout.tsx)
const initialState: AppState = {
    customFontFamily: null,
    fontWeight: 400,
    secondsScale: 82,
    thinOverride: false,
    monoEnforce: true,
    nightMode: false,
    glowColor: 'green',
    burninGuard: false,
    burninCrtMode: false,
    isFullscreen: false,
    settingsOpen: false,
    colonPhase: 0,
    lastColonToggle: 0,
    lastSecond: -1,
    lastRemainSecond: -1,
    monoWidthRatio: 0.6,
    driftX: 0,
    driftY: 0,
    driftDirX: 1,
    driftDirY: 1,
    driftTimer: 0,
    driftCycle: 0,
    lastDriftTime: 0,
    unchangedTime: 0,
    dlSpeed: 0,
    ulSpeed: 0,
    targetDl: 85,
    targetUl: 22,
    dlHistory: [],
    ulHistory: [],
    measuredDl: 0,
    measuredUl: 0,
    measureAvailable: false,
    longPressTimer: null,
    isPressing: false,
    publicIp: '--',
    localIp: '--',
    todos: [],
};

// Create the context with a default (empty) value
export const AppContext = createContext<AppContextType | undefined>(undefined);

// Custom hook to use the AppContext
export const useAppContext = () => {
    const context = useContext(AppContext);
    if (context === undefined) {
        throw new Error('useAppContext must be used within an AppProvider');
    }
    return context;
};

// AppProvider component (optional, but good for centralizing state logic)
// For now, the state is managed in Layout.tsx and passed directly
// If more complex state logic is needed, move state management here.
// export const AppProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
//     const [state, setState] = useState<AppState>(initialState);

//     const updateGlobalState = useCallback((newValues: Partial<AppState>) => {
//         setState(prevState => ({ ...prevState, ...newValues }));
//     }, []);

//     return (
//         <AppContext.Provider value={{ state, updateGlobalState }}>
//             {children}
//         </AppContext.Provider>
//     );
// };
