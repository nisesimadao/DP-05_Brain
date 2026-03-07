'use client';

import React, { useEffect, useRef, useCallback } from 'react';

export default function Calendar() {
    const calGridRef = useRef<HTMLDivElement>(null);

    const buildCalendar = useCallback(() => {
        const now = new Date();
        const year = now.getFullYear();
        const month = now.getMonth();
        const today = now.getDate();

        const monthNames = ['JAN', 'FEB', 'MAR', 'APR', 'MAY', 'JUN',
            'JUL', 'AUG', 'SEP', 'OCT', 'NOV', 'DEC'];
        const calMonthYear = `${monthNames[month]} ${year}`;

        // Weekday labels
        const days = ['SU', 'MO', 'TU', 'WE', 'TH', 'FR', 'SA'];
        const calWeekdaysHtml = days.map(d => `<span>${d}</span>`).join('');

        // Calculate grid
        const firstDay = new Date(year, month, 1).getDay();
        const daysInMonth = new Date(year, month + 1, 0).getDate();
        const totalCells = firstDay + daysInMonth;
        const totalRows = Math.ceil(totalCells / 7);
        const gridCells = totalRows * 7;

        let gridHtml = '';

        // Empty cells before first day of month
        for (let i = 0; i < firstDay; i++) {
            gridHtml += `<div class="cal-day empty"></div>`;
        }

        // Day cells
        for (let d = 1; d <= daysInMonth; d++) {
            const isToday = d === today ? ' today' : '';
            gridHtml += `<div class="cal-day${isToday}">${d}</div>`;
        }

        // Fill remaining cells in last row
        const remaining = gridCells - totalCells;
        for (let i = 0; i < remaining; i++) {
            gridHtml += '<div class="cal-day empty"></div>';
        }

        // Set the content using dangerouslySetInnerHTML
        // This is necessary because the original code constructs HTML strings
        return { calMonthYear, calWeekdaysHtml, gridHtml };
    }, []);

    const adjustCalendarRowHeights = useCallback(() => {
        if (!calGridRef.current) return;

        calGridRef.current.style.gridAutoRows = '10px';

        const cells = calGridRef.current.children.length;
        const totalRows = Math.max(1, Math.ceil(cells / 7));
        const rect = calGridRef.current.getBoundingClientRect();

        const rowH = Math.max(18, Math.floor(rect.height / totalRows));
        calGridRef.current.style.gridAutoRows = `${rowH}px`;
    }, []);

    // Initial calendar build and refresh on mount
    useEffect(() => {
        const { calMonthYear, calWeekdaysHtml, gridHtml } = buildCalendar();
        // Manually update DOM elements (or use state if you prefer)
        const monthYearEl = document.getElementById('calendar-month-year');
        if (monthYearEl) monthYearEl.textContent = calMonthYear;
        const weekdaysEl = document.getElementById('calendar-weekdays');
        if (weekdaysEl) weekdaysEl.innerHTML = calWeekdaysHtml;
        if (calGridRef.current) calGridRef.current.innerHTML = gridHtml;

        adjustCalendarRowHeights();

        // Schedule next calendar refresh for midnight
        const scheduleCalendarRefresh = () => {
            const now = new Date();
            const tomorrow = new Date(now);
            tomorrow.setDate(tomorrow.getDate() + 1);
            tomorrow.setHours(0, 0, 0, 0);
            const msUntilMidnight = tomorrow.getTime() - now.getTime();

            setTimeout(() => {
                buildCalendar(); // Rebuild for new day
                adjustCalendarRowHeights(); // Re-adjust after rebuild
                scheduleCalendarRefresh();
            }, msUntilMidnight);
        };
        scheduleCalendarRefresh();

        window.addEventListener('resize', adjustCalendarRowHeights, { passive: true });
        return () => {
            window.removeEventListener('resize', adjustCalendarRowHeights);
        };
    }, [buildCalendar, adjustCalendarRowHeights]);

    const { calMonthYear, calWeekdaysHtml, gridHtml } = buildCalendar();


    return (
        <div id="calendar-area">
            <div id="calendar-header">
                <span id="calendar-month-year">{calMonthYear}</span>
            </div>
            <div id="calendar-weekdays" dangerouslySetInnerHTML={{ __html: calWeekdaysHtml }}></div>
            <div id="calendar-grid" ref={calGridRef} dangerouslySetInnerHTML={{ __html: gridHtml }}></div>
        </div>
    );
}
