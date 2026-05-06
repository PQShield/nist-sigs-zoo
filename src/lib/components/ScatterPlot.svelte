<script lang="ts">
	import * as Plot from '@observablehq/plot';
	import { onMount } from 'svelte';
	import { dotColor, dotSymbol, dotTitle } from '$lib/plotHelpers';
	import { getFilterStore } from '$lib/filterStore';
	import { themeStore } from '$lib/themeStore';

	const { filteredRows } = getFilterStore();

	let container = $state<HTMLDivElement | null>(null);
	let width = $state(600);

	function render() {
		if (!container) return;
		const data = $filteredRows;
		const isDark = document.documentElement.classList.contains('dark');
		const color = (d: (typeof data)[0]) => dotColor(d, isDark);

		container.replaceChildren(
			Plot.plot({
				width,
				height: Math.min(width * 0.6, 500),
				x: { type: 'log', label: 'Public key size (bytes)', grid: true },
				y: { type: 'log', label: 'Signature size (bytes)', grid: true },
				style: {
					background: 'transparent',
					color: isDark ? '#C5CADA' : '#061128',
				},
				marks: [
					Plot.dot(data, {
						x: 'pk',
						y: 'sig',
						tip: true,
						title: dotTitle,
						stroke: color,
						symbol: dotSymbol,
						fill: color,
						fillOpacity: 0.8,
					}),
					Plot.crosshair(data, { x: 'pk', y: 'sig' }),
				],
			})
		);
	}

	onMount(() => {
		const ro = new ResizeObserver((entries) => {
			width = entries[0].contentRect.width || 600;
		});
		if (container) ro.observe(container);
		render();
		return () => ro.disconnect();
	});

	// Re-render whenever data or theme changes
	$effect(() => {
		$filteredRows; // track
		$themeStore;   // track
		render();
	});
</script>

<div bind:this={container} class="w-full [&_svg]:overflow-visible"></div>
