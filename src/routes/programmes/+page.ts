import { allKemData } from '$lib/kemSchemeData';
import { allProgrammes } from '$lib/programmeData';
import { programmeCards } from '$lib/programmePage';
import { allSchemeData } from '$lib/schemeData';

export function load() {
	return { cards: programmeCards(allProgrammes, allSchemeData, allKemData) };
}
