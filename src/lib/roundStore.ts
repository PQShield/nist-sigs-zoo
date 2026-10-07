import { writable } from 'svelte/store';
import type { StageFilter } from './types';

export type Round = 'round-1' | 'round-2' | 'round-3' | 'latest';

export const roundStore = writable<Round>('latest');

/** "Latest" and "Round 3" both select round-3 entrants; "Latest" shows their newest spec. */
export function roundToFilter(round: Round): { filter: StageFilter; useLatestVersion: boolean } {
	const stage = round === 'latest' ? 'round-3' : round;
	return {
		filter: { programme: 'nist-onramp', stage },
		useLatestVersion: round === 'latest' || round === 'round-3'
	};
}
