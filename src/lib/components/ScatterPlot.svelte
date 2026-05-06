<script lang="ts">
	import { onMount } from 'svelte';
	import { getFilterStore } from '$lib/filterStore';
	import { themeStore } from '$lib/themeStore';
	import type { ParameterSet } from '$lib/types';

	const { filteredRows } = getFilterStore();

	let container = $state<HTMLDivElement | null>(null);
	let viewHandle: { finalize(): void } | null = null;
	let renderSeq = 0;

	function colorCat(d: ParameterSet): string {
		if (d.classical) return 'classical';
		if (d.broken || d.warning) return 'broken';
		if (d.status === 'FIPS' || d.scheme === 'Falcon') return 'fips';
		return 'normal';
	}

	function shapeCat(d: ParameterSet): string {
		if (d.status === 'FIPS' || d.scheme === 'Falcon') return 'fips';
		if (d.classical) return 'classical';
		if (d.broken) return 'broken';
		return 'normal';
	}

	async function render() {
		if (!container) return;
		const seq = ++renderSeq;

		const { default: embed } = await import('vega-embed');
		const data = $filteredRows;
		const isDark = document.documentElement.classList.contains('dark');

		const values = data.map((d) => ({
			scheme: d.scheme,
			parameterset: d.parameterset,
			version: d.version || null,
			pk: d.pk,
			sig: d.sig,
			color: colorCat(d),
			shape: shapeCat(d),
			notes:
				[
					d.broken && !d.classical ? `⚠ ${d.broken}` : null,
					d.warning ? `⚠ ${d.warning}` : null
				]
					.filter(Boolean)
					.join(' | ') || null
		}));

		const colorRange = isDark
			? ['#9CA3AF', '#F87171', '#E09434', '#60A5FA']
			: ['#6B7280', '#DC2626', '#E09434', '#1D4ED8'];

		// eslint-disable-next-line @typescript-eslint/no-explicit-any
		const spec: any = {
			$schema: 'https://vega.github.io/schema/vega-lite/v5.json',
			width: 'container',
			height: 350,
			autosize: { type: 'fit-x', resize: true },
			data: { values },
			params: [
				{
					name: 'grid',
					select: { type: 'interval', encodings: ['x', 'y'] },
					bind: 'scales'
				}
			],
			mark: { type: 'point', filled: true, fillOpacity: 0.8, size: 80, strokeWidth: 1.5 },
			encoding: {
				x: {
					field: 'pk',
					type: 'quantitative',
					scale: { type: 'log' },
					axis: { title: 'Public key size (bytes)', grid: true, format: 's' }
				},
				y: {
					field: 'sig',
					type: 'quantitative',
					scale: { type: 'log' },
					axis: { title: 'Signature size (bytes)', grid: true, format: 's' }
				},
				color: {
					field: 'color',
					type: 'nominal',
					scale: { domain: ['classical', 'broken', 'fips', 'normal'], range: colorRange },
					legend: null
				},
				stroke: {
					field: 'color',
					type: 'nominal',
					scale: { domain: ['classical', 'broken', 'fips', 'normal'], range: colorRange },
					legend: null
				},
				shape: {
					field: 'shape',
					type: 'nominal',
					scale: {
						domain: ['fips', 'classical', 'broken', 'normal'],
						range: ['diamond', 'circle', 'triangle-down', 'square']
					},
					legend: null
				},
				tooltip: [
					{ field: 'scheme', type: 'nominal', title: 'Scheme' },
					{ field: 'parameterset', type: 'nominal', title: 'Parameter set' },
					{ field: 'version', type: 'nominal', title: 'Version' },
					{ field: 'pk', type: 'quantitative', title: 'pk (bytes)', format: ',' },
					{ field: 'sig', type: 'quantitative', title: 'sig (bytes)', format: ',' },
					{ field: 'notes', type: 'nominal', title: 'Notes' }
				]
			},
			config: {
				background: 'transparent',
				axis: {
					gridColor: isDark ? '#334155' : '#E2E8F0',
					tickColor: isDark ? '#475569' : '#94A3B8',
					labelColor: isDark ? '#94A3B8' : '#475569',
					titleColor: isDark ? '#C5CADA' : '#061128',
					domainColor: isDark ? '#475569' : '#94A3B8'
				},
				view: { stroke: 'transparent' }
			}
		};

		viewHandle?.finalize();
		const result = await embed(container, spec, { actions: false, renderer: 'svg' });
		if (seq !== renderSeq) {
			result.finalize();
			return;
		}
		viewHandle = result;
	}

	onMount(() => {
		return () => viewHandle?.finalize();
	});

	$effect(() => {
		$filteredRows;
		$themeStore;
		render();
	});
</script>

<div bind:this={container} class="w-full"></div>
<p class="mt-1 text-xs text-pqs-bluegray dark:text-pqs-steel">Scroll to zoom · drag to pan</p>
