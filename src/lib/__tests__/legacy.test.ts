import { describe, it, expect } from 'vitest';
import { legacyHomeRedirect, legacyPathRedirect, SIGNATURE_PARAMS } from '$lib/legacy';
import { encodeCompareFilter } from '$lib/compare';
import { SIGNATURE_FILTER } from '$lib/primitives';
import { roundFromParam, roundParam } from '$lib/roundStore';
import { defaultCommonFilter } from '$lib/compareFilters';
import type { FilterState, ProgrammeGroup } from '$lib/types';

describe('legacyHomeRedirect', () => {
	it('forwards a round + filter link to /signatures/ unchanged', () => {
		expect(legacyHomeRedirect('?r=2&s=MAYO,UOV&sort=sig', '', '')).toBe('/signatures/?r=2&s=MAYO,UOV&sort=sig');
	});
	it('keeps the hash', () => {
		expect(legacyHomeRedirect('?l=1,2', '#table', '')).toBe('/signatures/?l=1,2#table');
	});
	it('prefixes the deploy base path', () => {
		expect(legacyHomeRedirect('?r=1', '', '/nist-sigs-zoo')).toBe('/nist-sigs-zoo/signatures/?r=1');
	});
	it('forwards unknown round values rather than dropping them', () => {
		expect(legacyHomeRedirect('?r=7', '', '')).toBe('/signatures/?r=7');
	});
	it('returns null for a plain home visit', () => {
		expect(legacyHomeRedirect('', '', '')).toBeNull();
	});
	it('returns null for unrelated params such as analytics', () => {
		expect(legacyHomeRedirect('?utm_source=x', '', '')).toBeNull();
	});
});

describe('legacyPathRedirect', () => {
	it('moves /advanced/ with its axis params', () => {
		expect(legacyPathRedirect('signatures/advanced/', '?x=pkPlusSig&ys=linear', '', '')).toBe(
			'/signatures/advanced/?x=pkPlusSig&ys=linear'
		);
	});
	it('moves /history/ with base and hash', () => {
		expect(legacyPathRedirect('timeline/', '', '#2026-09-22', '/nist-sigs-zoo')).toBe('/nist-sigs-zoo/timeline/#2026-09-22');
	});
});

describe('SIGNATURE_PARAMS', () => {
	it('covers every key the signature URL codec can emit', () => {
		const defaults: FilterState = {
			...defaultCommonFilter(['nist-onramp', 'kpqc']),
			schemes: new Set(['A', 'B']),
			minPk: 1, maxPk: 10, minSig: 1, maxSig: 10, minPkPlusSig: 1, maxPkPlusSig: 10,
			minSigningCycles: 1, maxSigningCycles: 10, minVerificationCycles: 1, maxVerificationCycles: 10,
			minSigningUs: 0, maxSigningUs: Infinity, minVerificationUs: 0, maxVerificationUs: Infinity,
			sortCol: 'scheme', sortDir: 'asc'
		};
		const state: FilterState = {
			programmes: new Set<ProgrammeGroup>(['kpqc']), levels: new Set([1] as const), ngccLevels: new Set([128] as const),
			paramSets: 'all', hideBroken: true, hideImplementation: true, hideWarnings: true, chart: 'speed',
			schemes: new Set(['A']),
			minPk: 2, maxPk: 9, minSig: 2, maxSig: 9, minPkPlusSig: 2, maxPkPlusSig: 9,
			minSigningCycles: 2, maxSigningCycles: 9, minVerificationCycles: 2, maxVerificationCycles: 9,
			minSigningUs: 2, maxSigningUs: 9, minVerificationUs: 2, maxVerificationUs: 9,
			sortCol: 'pk', sortDir: 'desc'
		};
		const keys = [...encodeCompareFilter(SIGNATURE_FILTER, state, defaults).keys()];
		expect(keys.length).toBe(SIGNATURE_PARAMS.length - 1); // every key except the round param
		for (const k of keys) expect(SIGNATURE_PARAMS as readonly string[]).toContain(k);
	});

	it('forwards an old home link that carries a new filter key', () => {
		expect(legacyHomeRedirect('?p=kpqc', '', '')).toBe('/signatures/?p=kpqc');
	});
});

describe('round URL param', () => {
	it('round-trips every round, with no param for Latest', () => {
		for (const round of ['latest', 'round-1', 'round-2', 'round-3'] as const) {
			expect(roundFromParam(roundParam(round))).toBe(round);
		}
		expect(roundParam('latest')).toBeNull();
		expect(roundParam('round-2')).toBe('2');
		expect(roundFromParam('7')).toBe('latest');
	});
});
