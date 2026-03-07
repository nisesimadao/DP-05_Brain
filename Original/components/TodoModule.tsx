'use client';

import React, { useEffect } from 'react';
import { useAppContext, TodoItem } from '../context/AppContext';

export default function TodoModule() {
    const { state, updateGlobalState } = useAppContext();

    const fetchTodos = async () => {
        try {
            const res = await fetch('/api/todos');
            if (res.ok) {
                const data = await res.json();
                updateGlobalState({ todos: data });
            }
        } catch (e) {
            console.error('Failed to fetch todos', e);
        }
    };

    useEffect(() => {
        fetchTodos();
        const interval = setInterval(fetchTodos, 2000); // Polling for updates
        return () => clearInterval(interval);
    }, []);

    return (
        <div className="module" id="mod-todo">
            <div className="module-inner" id="todo-content">
                <div className="section-label">TODO LIST</div>
                <div id="todo-list" style={{
                    display: 'flex',
                    flexDirection: 'column',
                    gap: '4px',
                    marginTop: '8px',
                    overflowY: 'auto',
                    maxHeight: '100px'
                }}>
                    {state.todos.length === 0 ? (
                        <div style={{ fontSize: '10px', opacity: 0.4, fontStyle: 'italic' }}>NO TASKS</div>
                    ) : (
                        state.todos.map((todo: TodoItem) => (
                            <div key={todo.id} className="todo-item" style={{
                                display: 'flex',
                                alignItems: 'center',
                                gap: '8px',
                                fontSize: '11px',
                                opacity: todo.completed ? 0.4 : 1
                            }}>
                                <div style={{
                                    width: '6px',
                                    height: '6px',
                                    borderRadius: '50%',
                                    background: todo.completed ? 'transparent' : 'var(--text)',
                                    border: '1px solid var(--text)'
                                }}></div>
                                <span style={{
                                    textDecoration: todo.completed ? 'line-through' : 'none',
                                    whiteSpace: 'nowrap',
                                    overflow: 'hidden',
                                    textOverflow: 'ellipsis'
                                }}>{todo.text}</span>
                            </div>
                        ))
                    )}
                </div>
            </div>
        </div>
    );
}
