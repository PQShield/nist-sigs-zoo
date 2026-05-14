import { writable } from 'svelte/store';

export type Round = 'round-1' | 'round-2' | 'round-3' | 'latest';

export const roundStore = writable<Round>('latest');
