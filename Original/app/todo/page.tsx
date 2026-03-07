'use client';

import React, { useState, useEffect } from 'react';

export default function TodoPage() {
    const [todos, setTodos] = useState<any[]>([]);
    const [newTodo, setNewTodo] = useState('');

    const fetchData = async () => {
        try {
            const [todoRes, settingsRes] = await Promise.all([
                fetch('/api/todos'),
                fetch('/api/settings')
            ]);
            if (todoRes.ok) setTodos(await todoRes.json());
            if (settingsRes.ok) {
                const s = await settingsRes.json();
                document.documentElement.dataset.theme = s.nightMode ? 'night' : 'light';
                document.documentElement.dataset.glowColor = s.glowColor;
            }
        } catch (e) {
            console.error('Failed to fetch data', e);
        }
    };

    useEffect(() => {
        fetchData();
        const interval = setInterval(fetchData, 2000);
        return () => clearInterval(interval);
    }, []);

    const addTodo = async (e: React.FormEvent) => {
        e.preventDefault();
        if (!newTodo.trim()) return;
        await fetch('/api/todos', {
            method: 'POST',
            body: JSON.stringify({ action: 'add', text: newTodo })
        });
        setNewTodo('');
        fetchTodos();
    };

    const toggleTodo = async (id: string) => {
        await fetch('/api/todos', {
            method: 'POST',
            body: JSON.stringify({ action: 'toggle', id })
        });
        fetchTodos();
    };

    const deleteTodo = async (id: string) => {
        await fetch('/api/todos', {
            method: 'POST',
            body: JSON.stringify({ action: 'delete', id })
        });
        fetchTodos();
    };

    return (
        <div style={{
            maxWidth: '500px',
            margin: '40px auto',
            padding: '20px',
            fontFamily: 'var(--font-geist-sans), sans-serif',
            backgroundColor: 'var(--bg-dark)',
            borderRadius: '12px',
            boxShadow: '0 4px 20px rgba(0,0,0,0.1)',
            color: 'var(--text)'
        }}>
            <h1 style={{ letterSpacing: '2px', textAlign: 'center', fontSize: '24px', marginBottom: '20px' }}>DP-05 TODO</h1>

            <form onSubmit={addTodo} style={{ display: 'flex', gap: '8px', marginBottom: '20px' }}>
                <input
                    type="text"
                    value={newTodo}
                    onChange={(e) => setNewTodo(e.target.value)}
                    placeholder="NEW TASK..."
                    style={{
                        flex: 1,
                        padding: '12px',
                        border: '1px solid #ccc',
                        borderRadius: '4px'
                    }}
                />
                <button type="submit" style={{
                    padding: '12px 24px',
                    background: '#111',
                    color: '#fff',
                    border: 'none',
                    borderRadius: '4px',
                    cursor: 'pointer'
                }}>ADD</button>
            </form>

            <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
                {todos.map(todo => (
                    <div key={todo.id} style={{
                        display: 'flex',
                        alignItems: 'center',
                        gap: '12px',
                        padding: '12px',
                        backgroundColor: 'var(--bg)',
                        borderRadius: '8px',
                        border: '1px solid var(--grid-line)',
                        borderLeft: todo.completed ? '4px solid var(--grid-line)' : '4px solid var(--text)'
                    }}>
                        <input
                            type="checkbox"
                            checked={todo.completed}
                            onChange={() => toggleTodo(todo.id)}
                            style={{ width: '20px', height: '20px', cursor: 'pointer' }}
                        />
                        <span style={{
                            flex: 1,
                            textDecoration: todo.completed ? 'line-through' : 'none',
                            opacity: todo.completed ? 0.5 : 1
                        }}>{todo.text}</span>
                        <button
                            onClick={() => deleteTodo(todo.id)}
                            style={{ background: 'none', border: 'none', cursor: 'pointer', color: '#ff4d4d', fontSize: '14px' }}
                        >DEL</button>
                    </div>
                ))}
            </div>
        </div>
    );
}
