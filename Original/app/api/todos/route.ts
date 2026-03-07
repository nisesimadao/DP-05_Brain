import { NextResponse } from 'next/server';
import fs from 'fs/promises';
import path from 'path';

export const dynamic = 'force-dynamic';


const DB_PATH = path.join(process.cwd(), 'todos.json');

async function getTodos() {
    try {
        const data = await fs.readFile(DB_PATH, 'utf-8');
        return JSON.parse(data);
    } catch (e) {
        return [];
    }
}

async function saveTodos(todos: any) {
    await fs.writeFile(DB_PATH, JSON.stringify(todos, null, 2));
}

export async function GET() {
    const todos = await getTodos();
    return NextResponse.json(todos);
}

export async function POST(request: Request) {
    const body = await request.json();
    const todos = await getTodos();

    if (body.action === 'add') {
        const newTodo = {
            id: Math.random().toString(36).substr(2, 9),
            text: body.text,
            completed: false,
            createdAt: Date.now()
        };
        todos.push(newTodo);
    } else if (body.action === 'toggle') {
        const todo = todos.find((t: any) => t.id === body.id);
        if (todo) todo.completed = !todo.completed;
    } else if (body.action === 'delete') {
        const filtered = todos.filter((t: any) => t.id !== body.id);
        await saveTodos(filtered);
        return NextResponse.json(filtered);
    }

    await saveTodos(todos);
    return NextResponse.json(todos);
}
