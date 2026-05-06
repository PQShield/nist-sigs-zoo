import type { ParameterSet } from './types';

export function dotColor(d: ParameterSet, dark: boolean): string {
	if (d.classical)
		return dark ? '#94A3B8' : '#475569';       // slate-400 / slate-600
	if (d.broken || d.warning)
		return dark ? '#F87171' : '#DC2626';       // red-400 / red-600
	if (d.status === 'FIPS' || d.scheme === 'Falcon')
		return '#E09434';                          // apricot — readable both modes
	return dark ? '#7DD3FC' : '#064058';           // sky-300 / steel blue
}

export function dotSymbol(d: ParameterSet): string {
	if (d.status === 'FIPS' || d.scheme === 'Falcon') return 'star';
	if (d.classical) return 'circle';
	if (d.broken) return 'times';
	return 'plus';
}

export function dotTitle(d: ParameterSet): string {
	let str = `${d.scheme} ${d.parameterset}\npk: ${d.pk.toLocaleString()} B\nsig: ${d.sig.toLocaleString()} B`;
	if (d.broken && !d.classical) str += `\n⚠️ ${d.broken}`;
	if (d.warning) str += `\n⚠️ ${d.warning}`;
	return str;
}
