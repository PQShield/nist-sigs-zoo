import { writable } from 'svelte/store';

export type Round = 'round-1' | 'round-2';

export const roundStore = writable<Round>('round-2');
