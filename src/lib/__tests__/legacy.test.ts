import { describe, it, expect } from 'vitest';
import { legacyHomeRedirect, legacyPathRedirect, SIGNATURE_PARAMS } from '$lib/legacy';
import { buildUrlParams } from '$lib/filterStore';
import type { FilterState } from '$lib/types';

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
	it('covers every key buildUrlParams can emit', () => {
		const defaults: FilterState = {
			schemes: new Set(['A', 'B']),
			levels: new Set(['Pre-Quantum', 1, 2, 3, 4, 5] as const),
			minPk: 1, maxPk: 10, minSig: 1, maxSig: 10, minPkPlusSig: 1, maxPkPlusSig: 10,
			minSigningCycles: 1, maxSigningCycles: 10, minVerificationCycles: 1, maxVerificationCycles: 10,
			minSigningUs: 0, maxSigningUs: Infinity, minVerificationUs: 0, maxVerificationUs: Infinity,
			sortCol: 'scheme', sortDir: 'asc'
		};
		const state: FilterState = {
			schemes: new Set(['A']), levels: new Set([1] as const),
			minPk: 2, maxPk: 9, minSig: 2, maxSig: 9, minPkPlusSig: 2, maxPkPlusSig: 9,
			minSigningCycles: 2, maxSigningCycles: 9, minVerificationCycles: 2, maxVerificationCycles: 9,
			minSigningUs: 2, maxSigningUs: 9, minVerificationUs: 2, maxVerificationUs: 9,
			sortCol: 'pk', sortDir: 'desc'
		};
		const keys = [...buildUrlParams(state, defaults).keys()];
		expect(keys.length).toBe(19 - 1); // every key except the round param
		for (const k of keys) expect(SIGNATURE_PARAMS as readonly string[]).toContain(k);
	});
});
