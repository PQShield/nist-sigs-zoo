import { writable } from 'svelte/store';
import type { StageFilter } from './types';

export type Round = 'round-1' | 'round-2' | 'round-3' | 'latest';

export const roundStore = writable<Round>('latest');

/** "Latest" and "Round 3" both select round-3 entrants; "Latest" shows their newest spec and, unlike the pinned rounds, also includes other programmes' schemes. */
export function roundToFilter(round: Round): {
	filter: StageFilter;
	useLatestVersion: boolean;
	includeOtherProgrammes: boolean;
} {
	const stage = round === 'latest' ? 'round-3' : round;
	return {
		filter: { programme: 'nist-onramp', stage },
		useLatestVersion: round === 'latest' || round === 'round-3',
		includeOtherProgrammes: round === 'latest'
	};
}

/** URL `r` value of a round; null for Latest (no param). */
export function roundParam(round: Round): string | null {
	return round === 'latest' ? null : round.slice('round-'.length);
}

export function roundFromParam(v: string | null): Round {
	return v === '1' || v === '2' || v === '3' ? `round-${v}` : 'latest';
}
