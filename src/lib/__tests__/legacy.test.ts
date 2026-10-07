import { describe, it, expect } from 'vitest';
import { legacyHomeRedirect, legacyPathRedirect } from '$lib/legacy';

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
