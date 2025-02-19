/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Exceptions.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msitni <msitni@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/21 22:07:03 by msitni            #+#    #+#             */
/*   Updated: 2024/11/22 10:08:35 by msitni           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Exceptions.hpp"

const char* NotImplemented::what() const throw()
{
    return "NotImplemented";
}

const char* ImpossibleToReach::what() const throw()
{
    return "ImpossibleToReach";
}